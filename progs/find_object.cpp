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

// Object groups with type.
// Used for constructing filenames for reading data.
// Type was important if one does not want to use type
// substitution table and read all types directly.
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
  {"RautatiePiste",       "point"},
  {"RautatieViiva",        "line"}, // Railroads (lines)
  {"SahkoLinja",           "line"}, // Power lines
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
  pr.usage("[<options>] <src_dir> <lat> <lon>");
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

    if (args.size() != 3) usage();
    std::string in_dir = args[0];
    dPoint pt0(str_to_type<double>(args[2]), str_to_type<double>(args[1]));

    /********************************************************/

    // map rectangle in ETRS-TM35FIN
    ConvGeo cnv("ETRS-TM35FIN");
    cnv.bck(pt0);
    std::string name = pt_to_nom_fi(pt0, SC_FI_100k);

    // unzip data to a temporary dir
    std::string in_file = in_dir + "/" + name + ".zip";
    std::cerr << "reading data: " << in_file << "\n";
    TmpDir tmp_dir("shp2vmap_XXXXXX");
    tmp_dir.unzip(in_file);

    // go through all files
    for (const auto & f:file_glob({tmp_dir.get_dir() + "/*.shp"})){

      // open shape databases
      std::cerr << "  reading file: " << f << "\n";
      ShapeDB DB(f, 0);

      // go through all objects
      for (size_t i = 0; i<DB.dbf_num(); ++i){
        Opt opts = DB.get_opts(i);
        auto crds = DB.get(i);

        dPoint vec, pt(pt0);
        double dist = nearest_pt(crds, vec, pt, 100);
        if (dist>=100) continue;

        //print object
          std::cerr << f << " n:" << i << " npts:" << crds.npts() << " len:" << crds.length() << "\n";
        for (const auto o: opts)
            if (o.second!="" && o.second!="0") std::cerr << "  " << o.first << " " << o.second << "\n";
      }
    }

    /********************************************************/
    // Read detailed files
    name = pt_to_nom_fi(pt0, SC_FI_25k);
    std::vector<std::string> names;
    names.push_back(name+"L");
    names.push_back(name+"R");
    for (const auto & name: names){
      std::string zip = in_dir + "/" + name + ".shp.zip";

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
          case 's': ftype = "point"; break;
          case 'p': ftype = "area";  break;
          case 't': ftype = "text";  break;
          case 'v': ftype = "line";  break;
          default: throw Err() << "unknown type: " << f;
        }

        // open shape databases
        ShapeDB DB(path + base, 0);

        for (size_t i = 0; i<DB.dbf_num(); ++i){
          Opt opts = DB.get_opts(i);
          auto crds = DB.get(i);

          dPoint vec, pt(pt0);
          double dist = nearest_pt(crds, vec, pt, 100);
          if (dist==100) continue;

          //print object
          std::cerr << ftype << " n:" << i << " npts:" << crds.npts() << " len:" << crds.length() << "\n";
          for (const auto o: opts)
            if (o.second!="" && o.second!="0") std::cerr << "  " << o.first << " " << o.second << "\n";
        }
      }
    }

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
