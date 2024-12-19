///\cond HIDDEN (do not show this in Doxyden)

//#include <string>
//#include <vector>

#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shp.h"
#include "shape/dbf.h"
#include "geo_data/conv_geo.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"

using namespace std;

// Object groups with type
std::map<std::string, std::string> map_groups = {
  {"AmpumaRaja",           "line"},
  {"HallintoalueRaja",     "line"},
  {"HallintoAlue",         "area"},
  {"KarttanimiPiste",      "text"},
  {"KiitotieViiva",        "line"},
  {"KorkeusViiva",         "line"}, // Contours (lines)
  {"KoskiPiste",          "point"},
  {"KoskiViiva",           "line"},
  {"LiikenneAlue",         "area"},
  {"MaaAlue",              "area"},
  {"Maasto1Reuna",         "line"},
  {"Maasto1Viiva",         "line"},
  {"Maasto2Alue",          "area"},
  {"MajakkaPiste",        "point"},
  {"MetsaRaja",            "line"},
  {"PeltoAlue",            "area"},
  {"PorttiPiste",         "point"},
  {"RajavyohykeRaja",      "line"},
  {"RakennusAlue",         "area"},
  {"RakennusPiste",       "point"},
  {"RakennusViiva",        "line"}, // Some constructions (lines): fence, ski lift, etc.
  {"RautatiePiste",       "point"}, // Railroads (lines)
  {"RautatieViiva",        "line"},
  {"SahkoLinja",           "line"},
  {"SahkoPiste",          "point"},
  {"SuojaAlue",            "area"},
  {"SuojametsaRaja",       "line"},
  {"SuojeluAlue",          "area"},// Borders of natural areas (polygons)
  {"TaajamaAlue",          "area"},
  {"TiePiste",            "point"},// Road symbols?
  {"TieViiva",             "line"}, // Roads (lines)
  {"VesiAlue",             "area"},
  {"VesiliikenneViiva",    "line"},
  {"VesirakennelmaPiste", "point"},
  {"VesirakennelmaViiva",  "line"},
  {"VesiViiva",            "line"},
};

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("<options> <dir>");
  throw Err();
}


int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","VERB"});

    vector<string> files;
    Opt O = parse_options_all(&argc, &argv, options, {}, files);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (files.size() != 1) usage();
    auto dir = files[0];

    VMap2 vmap;
    ConvGeo cnv("ETRS-TM35FIN");

    for (const auto & map_group: map_groups){
      std::string base = dir + "/" + dir + "_" + map_group.first;

      std::string shp = base + ".shp";
      if (!file_exists(shp)) {
        std::cout << base << ": no SHP file\n";
        continue;
      }

      Shp SH(shp.c_str(), 0);

      // DBF file

      std::string dbf = base + ".dbf";
      if (!file_exists(dbf)) {
        std::cout << base << ": no DBF file\n";
        continue;
      }

      Dbf DB(dbf.c_str(), 0);

      int nr = DB.nrecords();
      int nf = DB.nfields();

      if (SH.get_num() != nr) {
        std::cout << base << ": different number of objects in SHP and DBF";
        continue;
      }

      for (size_t i = 0; i<nr; ++i){

        // collect object info to obj
        Opt o;
        for (size_t j = 0; j<nf; ++j)
          o.put<std::string>(DB.field_name(j), DB.get_str(i,j));

        // map type
        std::string type = o.get("Kohdeluokk","");

        // text objects do not have type
        if (map_group.second == "text") type = "1";

        if (type == "") {
          std:cerr << "can't parse object type: "
                   << base << ": " << i << "\n";
          continue;
        }

        // create object and set coordinates
        VMap2obj obj(map_group.second + ":" + type);
        obj.dMultiLine::operator=(SH.get(i));

        // contour names
        if (o.exists("Korkeus")){ 
          double k = o.get<double>("Korkeus");
          if (k>-1000 && k<10000)
            obj.name = o.get("Korkeus");
          if (int(k)%50 == 0)
            obj.set_type("line:52101");
        }

        // text object
        if (map_group.second == "text"){
          obj.name = o.get("text");
          // set refpoint

          int font = o.get<int>("textfont"); // 10 or 20
          int size = o.get<int>("textsize"); // ~150..400
          int col = o.get<int>("textcolour"); // 10 or 20
          std::string refs = o.get("placelocat"); // 523942.239;7548297.23

          if (col==20) obj.set_type("text:2");
          obj.scale=size/200.0;

          size_t n = refs.find(";");
          if (n!=string::npos){
            auto a = refs.substr(0,n);
            auto b = refs.substr(n+1,refs.size());
            std::cerr << "<" << refs << "> <" << a << ">+<" << b <<">\n";
            obj.ref_pt = dPoint(str_to_type<double>(a), str_to_type<double>(b));
            obj.ref_type = VMap2obj::make_type("point:0");
            cnv.frw(obj.ref_pt);
          }


        }


        cnv.frw(obj);
        vmap.add(obj);
      }
    }
    vmap2_export(vmap, VMap2types(Opt()), dir + ".vmap2", Opt());
  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

///\endcond
