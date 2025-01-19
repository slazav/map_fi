///\cond HIDDEN (do not show this in Doxyden)

#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_data/geo_utils.h"
#include "filename/filename.h"

#include "lib.h"
#include <regex>

using namespace std;

// Find difference between vmap files

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "vmap_diff");
  pr.name("Find difference between vmap files");
  pr.usage("[<options>] <file1> <file2>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD","OUT"});
  throw Err();
}

/**********************************************************/

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","OUT"});

    vector<string> files;
    Opt O = parse_options_all(&argc, &argv, options, {}, files);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);
    if (files.size() != 2) usage();
    auto file1 = files[0];
    auto file2 = files[1];

    if (!O.exists("out")) throw Err() << "output file not set (--out option)";
    auto ofile = O.get("out");


    // import both files (no type information)
    VMap2 vmap1, vmap2;
    if (!file_exists(file1)) throw Err() << "Can't find file: " << file1;
    if (!file_exists(file2)) throw Err() << "Can't find file: " << file2;
    VMap2types types;
    vmap2_import(file1, types, vmap1, Opt());
    vmap2_import(file2, types, vmap2, Opt());

    VMap2 vmap_diff;

    double search_dist = 2000; // where we are searching for modified objects [m]
    double match_dist = 100; // which point shift is treated as no change [m] 
    size_t count_same  = 0; // number of same objects
    size_t count_moved = 0; // number of objects with changed coordinates

    // Pass1: go through vmap1, find same/moved objects (no other parameter changes)
    vmap1.iter_start();
    while (!vmap1.iter_end()){
      auto it1 = vmap1.iter_get_next();
      auto i1 = it1.first;
      auto o1 = it1.second;

      // find nearest object with same headers, same number of points
      dRect rng = o1.bbox();
      // here we can use approximate degree range
      rng.expand(search_dist * 180/M_PI/6380000);
      uint32_t minid = -1;
      double mindist = +INFINITY;
      for (const auto & i2: vmap2.find(o1.type, rng)){
        double d = geo_maxdist_2d(o1,vmap2.get(i2));
        // (d is +inf for different npts, nsegments, headers)
        if (d<mindist){ mindist = d; minid = i2;}
      }
      if (mindist>search_dist) continue;
      // object within search_dist is found!

      if (mindist<match_dist){
        // same objects
        count_same++;
      }
      // object with changed coordinates
      else {
        o1.opts.put("diff_crds", dMultiLine(vmap2.get(minid)));
        vmap_diff.add(o1);
        count_moved++;
      }
      vmap1.del(i1);
      vmap2.del(minid);
    }
    std::cerr << "same  objects: " << count_same  << "\n";
    std::cerr << "moved objects: " << count_moved << "\n";
    std::cerr << "unprocessed 1: " << vmap1.size() << "\n";
    std::cerr << "unprocessed 2: " << vmap2.size() << "\n";
    vmap2_export(vmap_diff, types, ofile, Opt());
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

///\endcond
