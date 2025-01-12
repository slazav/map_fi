#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "geo_data/conv_geo.h"
#include "geo_data/geo_utils.h"
#include "geom/poly_tools.h"
#include "geo_nom/geo_nom_fi.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"

using namespace std;

// find objects in the original data

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <src_dir> <lat> <lon>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}


int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","VERB"});

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 2) usage();
    dPoint pt0(str_to_type<double>(args[1]), str_to_type<double>(args[0]));

    /********************************************************/

    ConvGeo cnv("ETRS-TM35FIN");
    dPoint p1(pt0);
    cnv.bck(p1);
    std::string name = pt_to_nom_fi(p1, SC_FI_100k);

    // go through all files
    for (const auto & f:file_glob({name + "*.vmap2", name + "*.vmap2.gz"})){

      std::cout << "### file: " << f << "\n";
      VMap2 vmap;
      vmap2_import(f, VMap2types(), vmap, Opt());

      // go through all objects
      vmap.iter_start();
      while (!vmap.iter_end()){
        auto pi = vmap.iter_get_next();
        auto obj = pi.second;

        dPoint vec, pt(pt0);
        double dist = nearest_vertex(obj, pt, (dPoint*)NULL, geo_dist_2d);
        if (dist>=100) continue;

        //print object
        std::cout << f << " npts:" << obj.npts() << " len:" << obj.length() << "\n";
        std::cout << obj << "\n\n";
      }
    }

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
