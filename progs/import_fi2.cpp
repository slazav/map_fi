#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shape_db.h"
#include "geo_data/conv_geo.h"
#include "tmpdir/tmpdir.h"
#include "geom/poly_tools.h"

#include "geo_nom/geo_nom_fi.h"

#include "lib.h"

using namespace std;

// Read original zip archive <name>.shp.zip with shape files from MML,
// convert to VMAP with all reasonable data.
// No configuration for specific types involved.
// I want this step to have intermediate "original" files for fast
// experiments with them.

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "import_fi2");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <name> <data_dir>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}

/************************************************/

// detect object class
std::string detect_class(const std::string & base){
  if (base.size()<2) throw Err() << "bad filename: " << base;
  switch (base[base.size()-1]){
    case 's': return "point:";
    case 'p': return "area:";
    case 't': return "text:";
    case 'v': return "line:";
  }
  throw Err() << "unknown type: " << base;
}

// parse <x>;<y> string for [D1] text objects
dPoint
parse_crd(const std::string & s){
  size_t n = s.find(";");
  if (n==string::npos) return dPoint();
  auto a = s.substr(0,n);
  auto b = s.substr(n+1,s.size());
  if (a.size()==0 || b.size()==0) return dPoint();
  return dPoint(str_to_type<double>(a), str_to_type<double>(b));
}

/************************************************/

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","VERB"});

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 2) usage();
    std::string name = args[0];
    std::string data_dir = args[1];

    /********************************************************/

    VMap2 vmap;
    ConvGeo cnv("ETRS-TM35FIN");
    dRect box = nom_to_range_fi(name);

    // unzip data to a temporary dir
    TmpDir tmp_dir("shp2vmap_XXXXXX");

    const char LR[] = "LR";
    const char I234[] = "1234";
    for (int n1 = 0; n1<4; ++n1){
      for (int n2 = 0; n2<4; ++n2){
        for (int n3 = 0; n3<2; ++n3){
          std::string ifile = data_dir + "/" + name + I234[n1] + I234[n2] + LR[n3] +".shp.zip";
          if (!file_exists(ifile)) {
            std::cerr << "skip src: " << ifile << "\n";
            continue;
          }
          std::cerr << "read src: " << ifile << "\n";
          tmp_dir.unzip(ifile);
        }
      }
    }

    /********************************************************/

    // go through all files
    for (const auto & f: file_glob({tmp_dir.get_dir() + "/*.shp"})){
      std::cerr << "reading file: " << f << "\n";
      auto path = file_get_prefix(f);
      auto base = file_get_basename(f, ".shp");
      if (base.size()<1) throw Err() << "bad basename: " << f;

      // get object class (point, text, line, area) from database name
      std::string cl = detect_class(base);
      // open shape databases
      ShapeDB DB(path + base, 0);
      if (DB.shp_num() != DB.dbf_num())
        throw Err() << "different number of objects in SHP and DBF: " << base;

      // go through all objects
      for (size_t i = 0; i<DB.shp_num(); ++i){
        Opt opts = DB.get_opts(i);
        std::string type = cl + opts.get("LUOKKA","1"); // "1" for text objects
        VMap2obj obj(type);

        auto crds = DB.get(i);
        if (crds.npts()==0) continue;

        //opts.put("file", base);

        // erase options with empty value
        for (auto i=opts.begin(); i!=opts.end();){
          if (i->second == "") i=opts.erase(i);
          else ++i;
        }

        // type number
        if (opts.exists("LUOKKA")){
          opts.put("type1", cl + opts.get("LUOKKA"));
          opts.put("type2", cl + opts.get("RYHMA") + "-" + opts.get("LUOKKA"));
          opts.erase("LUOKKA"); // type for F2
          opts.erase("RYHMA");  // type group for F2
        }

        // erase some defaults
        opt_del_def(opts, "ALUEJAKOON", "0");
        opt_del_def(opts, "ATTR3",      "0");
        opt_del_def(opts, "FROMLEFT",   "0");
        opt_del_def(opts, "FROMRIGHT",  "0");
        opt_del_def(opts, "KARTOGLK",   "0");
        opt_del_def(opts, "KORARV",   "0.0");
        opt_del_def(opts, "KORKEUS", "0.000");
        opt_del_def(opts, "KUNTA_NRO",  "0");
        opt_del_def(opts, "PAALLY",     "0");
        opt_del_def(opts, "PITUUS",     "0");
        opt_del_def(opts, "PYSYVAID",   "0");
        opt_del_def(opts, "SUUNTA",     "0"); // important for angle conversion
        opt_del_def(opts, "TASTAR",     "0");
        opt_del_def(opts, "TIENUM",     "0");
        opt_del_def(opts, "TIEOSA",     "0");
        opt_del_def(opts, "TOLEFT",     "0");
        opt_del_def(opts, "TORIGHT",    "0");
        opt_del_def(opts, "VALMAS",     "0");
        opt_del_def(opts, "VAPKOR",     "0");
        opt_del_def(opts, "YKSSUU",     "0");

        if (opts.exists("SUUNTA")){ // F2, points with directions
          obj.angle = -opts.get<double>("SUUNTA")/180.0;
          opts.erase("SUUNTA");
        }
        opt_mv_dbl(opts, "KORKEUS", "height"); // height
        opt_mv_txt(opts, "PITUUS", "length");
        opt_mv_txt(opts, "PAALLY",  "sidewalk");

        // text point/ref point for F2
        if (opts.exists("SIIRT_DX") && opts.exists("SIIRT_DY")){
          obj.ref_pt = crds[0][0];
          crds[0][0] += dPoint(opts.get<double>("SIIRT_DX")/1000.0,
                               opts.get<double>("SIIRT_DY")/1000.0);
          opts.erase("SIIRT_DX");
          opts.erase("SIIRT_DY");
        }

        /*************************************************/

        obj.opts = opts;

        // extend object beyond map bounaries to avoid side effects on the border
        // filter points
        if (cl == "line:" || cl == "area:"){
          line_filter_rdp(crds, 10); // m
          if (crds.npts()==0) continue;
          extend_object(crds, box);
        }

        if (obj.ref_pt != dPoint()) cnv.frw(obj.ref_pt);
        obj.dMultiLine::operator=(cnv.frw_acc(crds));
        vmap.add(obj);
      }
    }
    vmap2_export(vmap, VMap2types(), name + "_fi2.vmap2.gz", Opt());

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
