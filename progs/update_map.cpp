///\cond HIDDEN (do not show this in Doxyden)

#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_data/geo_utils.h"
#include "filename/filename.h"

#include "lib.h"
#include <regex>

using namespace std;

// Update local map from Label/Extra map. Do some object conversions.
// - Use tag ORG to mark original reference points, keep other destination objects untouched.
// - If reference point exists in both places - do nothing.
// - If reference point exists only in the source file - transfer point and labels
// - If reference point exists only in the destination - remove it

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "update_text");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <in_file.vmap2> <out_file.vmap2>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD", "VMAP2", "A"});
  throw Err();
}

/**********************************************************/

// custom filter for vmaps
void filter_vmap(VMap2 & vmap, const std::string & src){

  // read object conversion table
  auto oconvs = read_oconv("oconvs.txt");


  // Create labels
  vmap.iter_start();
  while (!vmap.iter_end()){
    auto p = vmap.iter_get_next();
    auto id = p.first;
    auto & obj = p.second;
    if (obj.is_class(VMAP2_TEXT)) continue;

    for (const auto & conv: oconvs){

      if (conv.src  != "*" && conv.src != src) continue;
      if (conv.type != "*" && !obj.is_type(conv.type)) continue;
      if (conv.lat  != "*" && conv.lon != "*"){
         dPoint pt(str_to_type<double>(conv.lon), str_to_type<double>(conv.lat));
         if (geo_dist_2d(obj.get_first_pt(), pt) > 200) continue;
      }

      auto n = std::regex_replace(
           obj.name, std::regex(conv.name_re), conv.name_subst);

      if (n == "" || n == "-"){
        //std::cerr << "oconv del: " << conv.type << " " << obj.name << "\n";
        vmap.del(id);
        break;
      }

      if (n!=obj.name){
        //std::cerr << "oconv sub: " << obj.name << " -> " << n << "\n";
        obj.name = n;
        vmap.put(id, obj);
      }
    }
  }
}

/**********************************************************/

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});
    ms2opt_add_vmap2t(options);
    options.add("src", 1, 0, "A", "Data source (label/extra, default: label)");

    vector<string> files;
    Opt O = parse_options_all(&argc, &argv, options, {}, files);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (files.size() != 2) usage();
    auto in_file = files[0];
    auto out_file = files[1];
    std::cout << "Updating text objects in " << out_file << " using " << in_file << "\n";

    // read file with type information if it's available
    VMap2types types(O);

    // import both files
    VMap2 vmap1, vmap2;
    if (!file_exists(in_file)) throw Err() << "Can't find file: " << in_file;
    vmap2_import(in_file, types, vmap1, Opt());

    if (file_exists(out_file))
      vmap2_import(out_file, types, vmap2, Opt());

    // set source from filename
    std::string src = file_get_basename(in_file);
    std::cerr << "Source: " << src << "\n";

    // filter objects
    filter_vmap(vmap1, src);

    // attach labels, get ref_id -> text_id multimap
    auto refs1 = vmap1.find_refs(100,200);
    auto refs2 = vmap2.find_refs(100,200);

    // Check all source objects
    auto i=refs1.begin();
    while (i!=refs1.end()){
      auto id = i->first;

      if (id == 0xFFFFFFFF){ // unconnected label, skip
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }
      auto o1 = vmap1.get(id); // ref point

      // find point in destination map with same type,
      // nearest coordinates, and same source
      dRect rng = o1.bbox(); rng.expand(2e-4);
      uint32_t minid = -1;
      double mindist = INFINITY;
      for (const auto & i2: vmap2.find(o1.type, rng)){
        auto o2 = vmap2.get(i2);
        if (o2.opts.get("Source") != src) continue;
        double d = dist(o1,o2); // inf for different npts, nsegments
        if (d<mindist){ mindist = d; minid = i2;}
      }
      // object found
      if (mindist<1e-4){
        auto i2 = minid;
        auto o2 = vmap2.get(i2);

        // update name and coordinates
        if (o2.name != o1.name)
          std::cout << "  update name: " << " " << o1.print_type()
                  << ": " << o2.name << " -> " << o1.name << " "
                  << o1.get_first_pt() << "\n";
        o2.name = o1.name;
        o2.set_coords(o1);
        vmap2.put(i2, o2);

        // update label names
        for (auto j=refs2.lower_bound(i2); j!=refs2.upper_bound(i2); ++j){
          auto l = vmap2.get(j->second);
          l.name = o1.name;
          vmap2.put(j->second, l);
        }

        // go to next object in vmap1
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }

      // if object is missing in vmap2, transfer it with all labels
      auto old_src = o1.opts.get("Source");
      o1.opts.put("Source", src);
      vmap2.add(o1);
      std::cout << "  add object: " << " " << o1.print_type()
                << ": " << o1.name << " " << o1.get_first_pt() << "\n";

      // transfer labels
      while (i!=refs1.end() && i->first == id){
        auto l = vmap1.get(i->second);
        vmap2.add(l);
        ++i;
      }
    }

    // Check all destination objects
    auto j=refs2.begin();
    while (j!=refs2.end()){
      auto id = j->first;

      if (id == 0xFFFFFFFF){ // skip unconnected label
        while (j!=refs1.end() && j->first == id) ++j;
        continue;
      }

      auto o2 = vmap2.get(id); // ref point

      // skip other sources
      if (o2.opts.get("Source") != src) {
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // Find nearest vmap1 object with same type
      uint32_t minid = -1;
      double mindist = INFINITY;
      dRect rng = o2.bbox(); rng.expand(2e-4);
      for (const auto & i1: vmap1.find(o2.type, rng)){
        auto o1 = vmap1.get(i1);
        double d = dist(o1,o2); // inf for different npts, nsegments
        if (d<mindist){ mindist = d; minid = i1;}
      }

      // if object found, do nothing
      if (mindist<1e-4){
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // if not, remove destination object and all labels
      std::cout << "  del object: " << VMap2obj::print_type(o2.type)
                << ": " << o2.name << " " << o2.get_first_pt() << "\n";
      vmap2.del(j->first);
      while (j!=refs2.end() && j->first == id){
        vmap2.del(j->second);
        ++j;
      }
    }

    do_update_labels(vmap2, types);
    vmap2_export(vmap2, types, out_file, Opt());
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

///\endcond
