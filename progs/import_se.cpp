#include <map>
#include <string>
#include <fstream>
#include "geom/multiline.h"
#include "geom/line_rectcrop.h"
#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_data/conv_geo.h"
#include "geo_data/geo_io.h"
#include "geo_nom/geo_nom_fi.h"
#include "vmap2/vmap2io.h"
#include "gis/gpkg.h"
#include "filename/filename.h"
#include "geom/poly_tools.h"
#include <regex>

#include "lib.h"

using namespace std;

// import Sweedish Topografi 50 Nedladdning vector data (gpkg files)

// text category translation
std::map<std::string, std::string> tr_textcat = {
 {"Anläggningsområde/Byggnadsanläggning",  "construction_area"},
 {"Bebyggelse",                            "building"},
 {"Fjällupplysningstext",                  "mountain_info"},
 {"Hydrografi",                            "hydrography"},
 {"Höjdkurvstext",                         "contour"},
 {"Kulturhistorisk lämning",               "monument"},
 {"Kyrka",                                 "church"},
 {"Skyddad natur",                         "protected_nature"},
 {"Terrängnamn",                           "terrain"},
 {"Tätort",                                "urban_area"},
 {"Upplysningstext",                       "information"},
};

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "import_no");
  pr.name("Read gpkg data from https://geotorget.lantmateriet.se");
  pr.usage("[<options>] <name> <data_dir>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 2) usage();
    std::string name  = args[0];
    std::string data_dir  = args[1];

    // expand cropping region be 100m to avoid side effects on the border
    dRect box = nom_to_range_fi(name);
    box.expand(100);

    auto ofile = name + "_se1.vmap2.gz";

    VMap2 vmap;
    dMultiLine brd;

    // go through all gpkg files in data_dir
    for (const auto & f: file_glob({data_dir + "/*.gpkg"})){
      std::cerr << "reading " << f << "\n";
      GPKG db(f);

      for (const auto & tt: db.get_tables()){
        std::cout << ">>> " << tt.first << "\n";

        ConvGeo cnv1(tt.second.srs, "ETRS-TM35FIN"); // se -> fi
        ConvGeo cnv2("ETRS-TM35FIN"); // fi -> wgs
        db.read_start(tt.first);
        while (1){
          auto obj = db.read_next();
          if (obj.type==-1) break;

          cnv1.frw(obj); // -> FI, pt2pt

          /*****************************************/
          // print all object types
          if (0){
            std::cout << obj.print_type() << " "
                      << obj.opts.get("objekttypnr") << " "
                      << obj.opts.get("objekttyp") << "\n";
          }
          // print all text types
          if (0){
            if (obj.opts.exists("textkategori"))
            std::cout << obj.print_type() << " "
                      << obj.opts.get("textkategori") << "\n";
          }
          // print text objects with different karttext and textstrang
          if (1){
            if (obj.opts.exists("textstrang") &&
                obj.opts.get("textstrang") != obj.opts.get("karttext"))
              std::cout << obj.opts.get("textstrang") << " -> "
                        << obj.opts.get("karttext") << "\n";
          }
          // save border
          //if (obj.opts.get("objekttypnr") == "1562"){
          //    auto crds = cnv2.frw_acc(dMultiLine(obj));
          //    brd.insert(brd.end(), crds.begin(), crds.end());
          //    continue;
          //}

          /*****************************************/


          // crop to FI map (area objects are closed lines!)
          obj.set_coords(rect_crop_multi(box, obj, obj.is_class(VMAP2_POLYGON)));
          if (obj.size()==0) continue;

          // remove defaults
          for (auto i=obj.opts.begin(); i!=obj.opts.end();){
            if (i->second == "") i=obj.opts.erase(i);
            else ++i;
          }

          // table name
          obj.opts.put("table", tt.first);

          // remove unneeded fields
          obj.opts.erase("objektidentitet");
          obj.opts.erase("vattendragsid");
          obj.opts.erase("skapad");
          obj.opts.erase("fid");

          // set type according to objekttypnr option
          if (obj.opts.exists("objekttypnr"))
            obj.set_type(obj.get_class(), obj.opts.get<int>("objekttypnr",1));

          // text types
          if (obj.opts.exists("karttext")){
            obj.set_type("text:1");

            if (obj.opts.get<double>("textriktning")!=0)
              obj.angle = obj.opts.get<double>("textriktning");

            // another way to set angle
            if (obj.size()==1 && obj.npts()>1){
            }
          }

          // translate text category
          if (obj.opts.exists("textkategori")){
            auto c=obj.opts.get("textkategori");
            if (tr_textcat.count(c)!=0)
              obj.opts.put("textkategori", tr_textcat[c]);
          }

          // filter points
          if (obj.get_class() == VMAP2_LINE ||
              obj.get_class() == VMAP2_POLYGON){
            line_filter_rdp(obj, 10); // m
            if (obj.npts()==0) continue;
          }
          obj.set_coords(cnv2.frw_acc(obj)); // -> WGS84, accurate
          vmap.add(obj);
        }
      }
    }
    if (vmap.size()) vmap2_export(vmap, VMap2types(), ofile, Opt());

    if (brd.size())  {
      GeoData D;
      D.push_back(GeoTrk(brd));
      write_gpx("brd.gpx", D);
    }

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
