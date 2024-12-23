#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shp.h"
#include "shape/dbf.h"
#include "geo_data/conv_geo.h"
#include "tmpdir/tmpdir.h"
#include "geom/poly_tools.h"

#include "geo_nom/geo_nom_fi.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"

using namespace std;

// Read original zip archives <name>/*.zip with shape files,
// append data to base vmap (create it if needed)


// Type substutution list. Used for:
// If destination is empty, type is silently skipped.
// If destination is '+' type is not substituded.
std::map<std::string, std::string> type_subst = {
 {"line:12121", "line:0x01"}, // Autotie IIa
 {"line:12122", "line:0x0B"}, // Autotie IIb
 {"line:12131", "line:0x02"}, // Autotie IIIa
 {"line:12132", "line:0x04"}, // Autotie IIIb
 {"line:12141", "line:0x06"}, // Ajotie
 {"line:12316", "line:0x0A"}, // Ajopolku
 {"line:12312", "line:0x16"}, // Talvitie
 {"line:12313", "line:0x2A"}, // Polku
 {"line:22312", "line:0x1A"}, // small power line
 {"line:44211", "line:0x19"}, // fence
 {"area:39110", "area:0x4F"}, // forest clearing

 {"point:45200", "point:0x6401"}, // gates
 {"point:45710", "point:0x2B03"}, // fire place
 {"point:45500", "point:0x6411"}, // wind generator
// {"line:42160",  "point:0x2B04"}, // huts
//point:45000 // watch tower

};


GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi (detailed data, <name>/*.zip)");
  pr.usage("[<options>] <name> <out_file>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD", "VMAP2"});
  throw Err();
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

    if (args.size() != 2) usage();
    auto in_dir = args[0];
    auto out_file = args[1];
    auto name = file_get_name(in_dir);

    VMap2 vmap, vmap_t;
    ConvGeo cnv("ETRS-TM35FIN");

    // map rectangle in ETRS-TM35FIN
    dRect brd = nom_to_range_fi(name);

    for (const auto & zip: file_glob({in_dir + "/*.zip"})){

      // unzip data to a temporary dir
      std::cerr << "reading file: " << zip << "\n";
      TmpDir tmp_dir("shp2vmap_XXXXXX");
      tmp_dir.unzip(zip);

      for (const auto & f: file_glob({tmp_dir.get_dir() + "/*.shp"})){
        std::cerr << "  reading file: " << f << "\n";
        auto path = file_get_prefix(f);
        auto base = file_get_basename(f, ".shp");
        if (base.size()<1) throw Err() << "bad basename: " << f;

        std::string ftype="";
        switch (base[base.size()-1]){
          case 's': ftype = "point:"; break;
          case 'p': ftype = "area:";  break;
          case 't': ftype = "text:";  break;
          case 'v': ftype = "line:";  break;
          default: throw Err() << "unknown type: " << f;
        }

        // skip text objects (see make_text prog)
        //if (ftype == "text:") continue;

        // open shape databases
        std::string shp = path + base + ".shp";
        std::string dbf = path + base + ".dbf";
        if (!file_exists(shp)) throw Err() << "no SHP file: " << shp;
        if (!file_exists(dbf)) throw Err() << "no DBF file: " << dbf;
        Shp SH(shp.c_str(), 0);
        Dbf DB(dbf.c_str(), 0);
        int nr = DB.nrecords();
        int nf = DB.nfields();
        if (SH.get_num() != nr)
          throw Err() << "different number of objects in SHP and DBF: " << base;

        // go through all objects
        for (size_t i = 0; i<nr; ++i){

          // collect object info to obj
          Opt o;
          for (size_t j = 0; j<nf; ++j)
            o.put<std::string>(DB.field_name(j), DB.get_str(i,j));

          // type
          std::string type = o.get("LUOKKA","");
          if (type == "") {
            std:cerr << "can't parse object type: "
                     << base << ": " << i << "\n";
            continue;
          }
          type = ftype + type;

          // check type
          if (type_subst.count(type)==0) 
            continue;
////          throw Err() << "Please add unknown type: " << type;

          // skip or substitute type
          if (type_subst[type]=="" || type_subst[type]=="-") continue;
          if (type_subst[type]!="+") type = type_subst[type];


          // create object and set coordinates
          VMap2obj obj(type);
          auto crds = SH.get(i);

          // when replacing line/area type to point, keep single coordinates
          if (ftype!="point:" && type.substr(0,6) == "point:" && crds.npts()>1) {
            dPoint pt = crds.bbox().cnt();
            crds.clear(); crds.add_point(pt);
          }
/*

          // extend objects bejond map bounaries
          for (auto & l:crds){
            for (auto i1=l.begin(); i1!=l.end(); ++i1){
              auto i2 = (i1+1 == l.end())? l.begin() : i1+1;
              if (brd.contains_n(*i1) && !brd.contains_n(*i2))
                *i2 = *i2 + norm(*i2-*i1)*100;
              if (brd.contains_n(*i2) && !brd.contains_n(*i1))
                *i1 = *i1 + norm(*i1-*i2)*100;
              // should we proccess points with both neighbours on the border?
            }
          }

*/
          // filter points
          double acc = 10.0; //m
          if (acc>0) {
            line_filter_rdp(crds, acc);
            if (crds.npts()==0) continue;
          }

          obj.dMultiLine::operator=(cnv.frw_acc(crds));
          vmap.add(obj);
        }
      }
    }

    // read file with type information if it's available
    VMap2types types(O);

    if (file_exists(out_file)) vmap2_import(out_file, types, vmap, Opt());
    vmap2_export(vmap, types, out_file, Opt());
  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
