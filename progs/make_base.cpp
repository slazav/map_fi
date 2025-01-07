#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shape_db.h"
#include "geo_data/conv_geo.h"
#include "tmpdir/tmpdir.h"
#include "geom/poly_tools.h"

#include "geo_nom/geo_nom_fi.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"
#include "vmap2/vmap2tools.h"

using namespace std;

// Read original zip archive <name>.zip with shape files,
// create base vmap (without text objects).

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <type_conv.txt> <data_dir> <name> <out_file>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD", "VMAP2"});
  throw Err();
}

/************************************************/

// detect data source from shape file name
int detect_src(const std::string & base){
  if (base.size()>3 && base[3]=='_') return 1;
  if (base.size()>1 && base[1]=='_') return 2;
  throw Err() << "unknown source type: " << base << "\n";
}

// detect object class for a Detailed shape file
std::string detect_class(const std::string & base){
  int s = detect_src(base);
  int n = base.length();
  if (s == 1){
    if (base.rfind("KarttanimiPiste") == n-15) return "text:";
    if (base.rfind("Piste") == n-5) return "point:";
    if (base.rfind("Viiva") == n-5) return "line:";
    if (base.rfind("Linja") == n-5) return "line:";
    if (base.rfind("Reuna") == n-5) return "line:";
    if (base.rfind("Raja")  == n-4) return "line:";
    if (base.rfind("Alue")  == n-4) return "area:";
  }
  else if (s == 2) {
    switch (base[base.size()-1]){
      case 's': return "point:";
      case 'p': return "area:";
      case 't': return "text:";
      case 'v': return "line:";
    }
  }
  throw Err() << "unknown type: " << base;
}

std::string detect_type(const Opt & opts){
  std::string ret = opts.get("Kohdeluokk",""); // [D1]
  if (ret != "") return ret;
  ret = opts.get("LUOKKA",""); // [D2]
  if (ret != "") return ret;
  std:cerr << "can't parse object type: " << opts << "\n";
  return ret;
}

/************************************************/


// extend objects beyond map bounaries
void extend_object(dMultiLine & crds, const dRect & box){
  for (auto & l:crds){
    for (auto i1=l.begin(); i1!=l.end(); ++i1){
      auto i2 = (i1+1 == l.end())? l.begin() : i1+1;
      if (box.contains_n(*i1) && !box.contains_n(*i2))
        *i2 = *i2 + norm(*i2-*i1)*100;
      if (box.contains_n(*i2) && !box.contains_n(*i1))
        *i1 = *i1 + norm(*i1-*i2)*100;
      // should we proccess points with both neighbours on the border?
    }
  }
}

// unzip all shape files for the map
void
unzip_all(TmpDir & tmp_dir, const std::string & data_dir, const std::string & name){
  auto f = data_dir + "/" + name + ".zip";
  std::cerr << "unzipping: " << f << "\n";
  tmp_dir.unzip(f);
  for (const auto & f: file_glob({data_dir + "/" + name + "???.shp.zip"})){
    std::cerr << "unzipping: " << f << "\n";
    tmp_dir.unzip(f);
  }
}


int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","VERB"});
    ms2opt_add_vmap2t(options);

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 4) usage();
    std::string tconv_f = args[0];
    std::string data_dir = args[1];
    std::string name = args[2];
    std::string out_file = args[3];

    /********************************************************/
    // read type conversions
    struct tconv_t {int n; std::string dst;};
    std::map<std::string, tconv_t> tconvs;

    std::ifstream ff(tconv_f);
    if (!ff) throw Err() << "can't open file: " << tconv_f;
    int line_num[2] = {0,0}; // line counter for read_words
    while (1){
      auto vs = read_words(ff, line_num, false);
      if (vs.size()==0) break;
      try{
        if (vs.size()!=3) throw Err()
          << "three words expected: <src_type> <source num> <dst_type>";
        tconvs[vs[0]] = {str_to_type<int>(vs[1]), vs[2]};
      }
      catch (Err & e) {
        throw Err() << tconv_f << ":" << line_num[0] << ": " << e.str();
      }
    }
    /********************************************************/

    VMap2 vmap;
    VMap2types types(O);

    /********************************************************/

    // map rectangle in ETRS-TM35FIN
    dRect brd = nom_to_range_fi(name);
    ConvGeo cnv("ETRS-TM35FIN");

    // unzip data to a temporary dir
    TmpDir tmp_dir("shp2vmap_XXXXXX");
    unzip_all(tmp_dir, data_dir, name);

    // go through all files
    for (const auto & f: file_glob({tmp_dir.get_dir() + "/*.shp"})){
      std::cerr << "reading file: " << f << "\n";
      auto path = file_get_prefix(f);
      auto base = file_get_basename(f, ".shp");
      if (base.size()<1) throw Err() << "bad basename: " << f;

      int src = detect_src(base);
      std::string cl = detect_class(base);
      if (cl == "text:") continue;

      // open shape databases
      ShapeDB DB(path + base, 0);
      if (DB.shp_num() != DB.dbf_num())
        throw Err() << "different number of objects in SHP and DBF: " << base;

      // go through all objects
      for (size_t i = 0; i<DB.shp_num(); ++i){
        Opt opts = DB.get_opts(i);
        std::string org_type = detect_type(opts);
        if (org_type == "") continue;
        std::string type = cl + org_type;

        // check type
        if (tconvs.count(type)==0){
          std::cerr << "unknown type: " << type << "\n";
          continue;
        }

        // skip or substitute type
        if (tconvs[type].n != src) continue;
        if (tconvs[type].dst=="" || tconvs[type].dst=="-") continue;
        if (tconvs[type].dst!="+") type = tconvs[type].dst;

        // create object and set coordinates
        VMap2obj obj(type);
        auto crds = DB.get(i);

        // when replacing line/area type to point, keep single coordinates
        if (cl!="point:" && type.substr(0,6) == "point:" && crds.npts()>1) {
          dPoint pt = crds.bbox().cnt();
          crds.clear(); crds.add_point(pt);
        }

        // contour names (D1)
        if (opts.exists("Korkeus") && type == "line:0x21"){
          double k = opts.get<double>("Korkeus");
          if (k>-1000 && k<10000)
            obj.name = opts.get("Korkeus");
          if (int(k)%50 == 0)
            obj.set_type("line:0x22");
        }

        // for trails (but not pedestrian roads!) treat Paallyste/PAALLY = 2 as a "bridge"
        // create additional object.
        if (opts.get("PAALLY") == "2" && type == "line:0x2A" && org_type!="12314"){
          VMap2obj obj1("line:0x08");
          obj1.dMultiLine::operator=(cnv.frw_acc(crds));
          vmap.add(obj1);
        }

        // extend objects beyond map bounaries
        if (src==1) extend_object(crds, brd);

        // filter points
        double acc = 10.0; //m
        if (acc>0){
          line_filter_rdp(crds, acc);
          if (crds.npts()==0) continue;
        }

        obj.dMultiLine::operator=(cnv.frw_acc(crds));
        vmap.add(obj);
      }
    }

    do_join_lines(vmap, 20, 30);
    vmap2_export(vmap, types, out_file, Opt());

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
