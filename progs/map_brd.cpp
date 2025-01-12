#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_nom/geo_nom_fi.h"

#include "geom/line.h"
#include "geo_data/geo_io.h"
#include "geo_data/conv_geo.h"

using namespace std;

// Write track with border of a map <name>

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "map_brd");
  pr.name("Write track with border of a map <name> to <name>.GPX");
  pr.usage("[<options>] <name>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});

    vector<string> files;
    Opt O = parse_options_all(&argc, &argv, options, {}, files);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (files.size() != 1) usage();
    auto name = files[0];

    ConvGeo cnv("ETRS-TM35FIN");
    dLine l = rect_to_line(nom_to_range_fi(name), true);

    GeoData D;
    D.trks.push_back(cnv.frw_acc(l));
    write_geo(name + ".GPX", D, O);
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

