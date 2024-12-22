///\cond HIDDEN (do not show this in Doxyden)

#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"

using namespace std;

// Transfer text objects (and reference points) from source (original text data)
// to the destination (overlay map).
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
  pr.opts({"HELP","POD","VMAP2"});
  throw Err();
}

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD","VERB"});
    ms2opt_add_vmap2t(options);

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
    vmap2_import(in_file, types, vmap1, Opt());
    vmap2_import(out_file, types, vmap2, Opt());

    // attach labels, get ref_id -> text_id multimap
    auto refs1 = vmap1.find_refs(10,100);
    auto refs2 = vmap2.find_refs(10,100);

    // Check all source objects
    auto i=refs1.begin();
    while (i!=refs1.end()){
      auto id = i->first;
      auto o1 = vmap1.get(id); // ref point

      // find point with same type, close coordinates, and tag ORG in vmap2
      bool found = false;
      dRect rng = o1.bbox(); rng.expand(1e-4);
      for (const auto & i2: vmap2.find(o1.type, rng)){
        auto o2 = vmap2.get(i2);
        if (o2.tags.count("ORG")==0) continue;

        found = true;
        // update name and coordinates
        if (o2.name != o1.name)
          std::cout << "  update name: " << o2.name << " -> " << o1.name << "\n";
        o2.name = o1.name;
        o2.set_coords(o1);
        vmap2.put(i2, o2);

        // update label names
        for (auto j=refs2.lower_bound(i2); j!=refs2.upper_bound(i2); ++j){
          auto l = vmap2.get(j->second);
          l.name = o1.name;
          vmap2.put(j->second, l);
        }
        break;
      }

      // go to next object
      if (found) {
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }

      // if object is missing in vmap2, transfer it with all labels
      o1.tags.insert("ORG");
      vmap2.add(o1);
      std::cout << "  add object: " << VMap2obj::print_type(o1.type) << ": " << o1.name << "\n";

      // transfer all labels
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
      auto o2 = vmap2.get(id); // ref point
      if (o2.tags.count("ORG")==0) {
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // Find point with same type and close coordinates in vmap1.
      // Use much smaller search tolerance, because coordinates should be
      // already updated, and we do not want to keep object which is close to existing one.
      dRect rng = o2.bbox(); rng.expand(1e-5);
      if (vmap1.find(o2.type, rng).size()){
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // remove object and all labels
      std::cout << "  del object: " << VMap2obj::print_type(o2.type) << ": " << o2.name << "\n";
      vmap2.del(j->first);
      while (j!=refs2.end() && j->first == id){
        vmap2.del(j->second);
        ++j;
      }
    }

    vmap2_export(vmap2, types, out_file, Opt());
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

///\endcond
