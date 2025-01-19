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
  pr.name("find difference or update objects");
  pr.usage("[<options>] <in_file.vmap2> <out_file.vmap2>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD", "VMAP2", "A"});
  throw Err();
}

/**********************************************************/

// custom filter for vmaps
void filter_vmap(VMap2 & vmap, const std::string & fname, const std::string & map){

  // read object conversion table
  auto oconvs = read_oconv(fname);

  double find_dist = 100; //m

  vmap.iter_start();
  while (!vmap.iter_end()){
    auto p = vmap.iter_get_next();
    auto id = p.first;
    auto & obj = p.second;
    if (obj.is_class(VMAP2_TEXT)) continue;

    auto pt = obj.get_first_pt();

    for (const auto & conv: oconvs){
      if (conv.size()<5) throw Err() << "too short conv list";
      if (conv[0] == "add") continue; // process later
      if (conv[0] != "*" && conv[0]!=map) continue; // map
      if (conv[1] != "*" && !obj.is_type(conv[1])) continue; // type
      if (conv[2] != "*" && conv[3] != "*"){ // lon-lat
         dPoint pt(str_to_type<double>(conv[2]), str_to_type<double>(conv[3]));
         if (geo_dist_2d(obj.get_first_pt(), pt) > find_dist) continue;
      }
      if (conv[4] != "*" && obj.name != conv[4]) continue; // name
      std::string action = conv[5];

      if (action == "del"){
        if (conv.size()!=6) throw Err() << "no arguments expected for del action";
        vmap.del(id);
        break;
      }

      if (action == "move"){
        if (conv.size()!=8) throw Err() << "two arguments expected for move action";
        obj.clear();
        obj.add_point(dPoint(str_to_type<double>(conv[6]), str_to_type<double>(conv[7])));
        vmap.put(id, obj);
        continue;
      }

      if (action == "rename"){
        if (conv.size()!=7) throw Err() << "one argument expected for rename action";
        obj.name = conv[6];
        vmap.put(id, obj);
        continue;
      }

      if (action == "regex"){
        if (conv.size()!=8) throw Err() << "two argument expected for regex action";
        obj.name = std::regex_replace(
           obj.name, std::regex(conv[6]), conv[7]);
        vmap.put(id, obj);
        continue;
      }

      if (action == "ch_type"){
        if (conv.size()!=7) throw Err() << "one argument expected for ch_type action";
        obj.set_type(conv[6]);
        vmap.put(id, obj);
        continue;
      }

      throw Err() << "Unknown action: " << action;

    }
  }

  // add actions
  for (const auto & conv: oconvs){
    if (conv.size()<5) throw Err() << "too short conv list";
    if (conv[0] != "add") continue;

    VMap2obj obj;
    obj.set_type(conv[1]);
    obj.add_point(dPoint(str_to_type<double>(conv[2]), str_to_type<double>(conv[3])));
    obj.name = conv[4];
    vmap.add(obj);
  }


}

/**********************************************************/

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});
    ms2opt_add_vmap2t(options);
    options.add("apply", 0, 'a', "A", "Instead of printing diff, update output file");
    options.add("map",   1, 'm', "A", "Map name");
    options.add("patch_file",   1, 'p', "A", "File with patch to apply to the input file");

    vector<string> files;
    Opt O = parse_options_all(&argc, &argv, options, {}, files);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (files.size() != 2) usage();
    auto in_file = files[0];
    auto out_file = files[1];

    bool apply = O.exists("apply");
    std::string pfile = O.get("patch_file", "oconvs.txt");
    std::string mapname = O.get("map", file_get_basename(out_file));


    // read file with type information if it's available
    VMap2types types(O);

    // import both files
    VMap2 vmap1, vmap2;
    if (!file_exists(in_file)) throw Err() << "Can't find file: " << in_file;
    vmap2_import(in_file, types, vmap1, Opt());
    std::cerr << "Reading file: " << in_file << ": " << vmap1.size() << " objects\n";

    if (file_exists(out_file)){
      vmap2_import(out_file, types, vmap2, Opt());
      std::cerr << "Reading file: " << out_file << ": " << vmap2.size() << " objects\n";
    }

    // filter objects
    filter_vmap(vmap1, pfile, mapname);
    std::cerr << "Filtering src file: " << vmap1.size() << " objects\n";

    // attach labels in the source file - for setting default label positions
    auto refs1 = vmap1.find_refs(100,200);

    double search_dist = 2000; // where we are searching for modified objects [m]
    double match_dist = 100; // which point shift is treated as no change [m].

    std::set<uint32_t> processed1, processed2;
    std::cout << std::fixed << std::setprecision(5);

    // Pass1: go through vmap1, find objects in vmap2 with same name
    vmap1.iter_start();
    while (!vmap1.iter_end()){
      auto it1 = vmap1.iter_get_next();
      auto i1 = it1.first;
      auto o1 = it1.second;
      auto pt1 = o1.get_first_pt();
      // work only with point objects
      if (!o1.is_class(VMAP2_POINT)) continue;

      // find nearest object with same type, same number of points
      dRect rng = o1.bbox();
      // here we can use approximate degree range
      rng.expand(search_dist * 180/M_PI/6380000);
      uint32_t minid = -1;
      double mindist = +INFINITY;
      for (const auto & i2: vmap2.find(o1.type, rng)){
        auto o2 = vmap2.get(i2);
        if (o2.opts.get("Source")=="")
          continue; // temporary, select objects with any non-empty Source setting
        if (processed2.count(i2)) continue;

        double d = geo_maxdist_2d(o1,o2);
        // (d is +inf for different npts, nsegments)
        if (d<mindist && o1.name==o2.name){ mindist = d; minid = i2;}
      }

      // closiest object with same name within search_dist
      if (mindist < search_dist){
        auto o2 = vmap2.get(minid);
        auto pt2 = o2.get_first_pt();
        //move
        if (mindist > match_dist) {
          if (!apply)
            std::cout << mapname << " "  << o1.print_type() << " "
               << pt1.x << " " << pt1.y << " \"" << o1.name
               <<  "\" move " << pt2.x << " " << pt2.y << "\n";
        }
        vmap2.put(minid, o1); // update object
        processed1.insert(i1);
        processed2.insert(minid);
        continue;
      }
    }

    // Pass2: go through vmap1, find objects in vmap2 with changed name
    //        transter unattached objects
    vmap1.iter_start();
    while (!vmap1.iter_end()){
      auto it1 = vmap1.iter_get_next();
      auto i1 = it1.first;
      auto o1 = it1.second;


      // select only unprocessed point objects
      if (processed1.count(i1)) continue;
      if (!o1.is_class(VMAP2_POINT)) continue;

      auto pt1 = o1.get_first_pt();

      // find nearest object with same type, same number of points
      dRect rng = o1.bbox();
      // here we can use approximate degree range
      rng.expand(search_dist * 180/M_PI/6380000);
      uint32_t minid = -1;
      double mindist = +INFINITY;
      for (const auto & i2: vmap2.find(o1.type, rng)){
        auto o2 = vmap2.get(i2);
        if (o2.opts.get("Source")=="")
          continue; // temporary, select objects with any non-empty Source setting
        if (processed2.count(i2)) continue;

        double d = geo_maxdist_2d(o1,o2);
        // (d is +inf for different npts, nsegments)
        if (d<mindist){ mindist = d; minid = i2;}
      }

      // closiest object with different name within search_dist
      if (mindist < search_dist){
        auto o2 = vmap2.get(minid);
        auto pt2 = o2.get_first_pt();
        //move
        if (mindist > match_dist) {
          if (!apply)
            std::cout << mapname << " " << o1.print_type() << " "
               << pt1.x << " " << pt1.y << " \"" << o1.name
               << "\" move " << pt2.x << " " << pt2.y << "\n";
          pt1 = pt2; // to print correct rename diff
        }
        // change name
        if (o1.name != o2.name) {
          if (!apply)
            std::cout << mapname << " "  << o1.print_type() << " "
               << pt1.x << " " << pt1.y << " \"" << o1.name
               << "\" rename \"" << o2.name << "\"\n";
        }

        vmap2.put(minid, o1); // update object
        processed1.insert(i1);
        processed2.insert(minid);
        continue;
      }

      // no object found -- add new
      if (!apply)
        std::cout << mapname << " " << o1.print_type() << " "
                  << pt1.x << " " << pt1.y << " \"" << o1.name
                  << "\" del\n";

      processed1.insert(i1);
      processed2.insert(vmap2.add(o1));

      // transfer labels
      for (auto i = refs1.find(i1); i!=refs1.end() && i->first == i1; ++i)
        vmap2.add(vmap1.get(i->second));
    }

    // Pass3: go through vmap2, delete unprocessed objects
    vmap2.iter_start();
    while (!vmap2.iter_end()){
      auto it2 = vmap2.iter_get_next();
      auto i2 = it2.first;
      auto o2 = it2.second;

      // select unprocessed point objects with non-empty Source option
      if (processed2.count(i2)) continue;
      if (!o2.is_class(VMAP2_POINT)) continue;
      if (o2.opts.get("Source")=="") continue;

      auto pt2 = o2.get_first_pt();
      if (!apply)
        std::cout << mapname << " "  << o2.print_type() << " "
                  << pt2.x << " " << pt2.y << " \"" << o2.name
                  << "\" add\n";

      vmap2.del(i2); // delete object
    }

    if (apply){
      std::cerr << "Saving file: " << out_file << "\n";
      do_update_labels(vmap2, types);
      vmap2_export(vmap2, types, out_file, Opt());
    }
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

///\endcond
