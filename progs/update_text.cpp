///\cond HIDDEN (do not show this in Doxyden)

#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"
#include "vmap2/vmap2tools.h"
#include "geo_data/geo_utils.h"

#include <regex>

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

/**********************************************************/

void replace_string(std::string & str, const std::string & s, const std::string & r){
  str = std::regex_replace(str, std::regex(s), r);
}

void change_name(VMap2obj & obj, uint32_t type, double lon, double lat, std::string s, std::string r){
  if (type!=-1 && obj.type != type) return;
  if (!std::isnan(lat) && !std::isnan(lat) && 
      geo_dist_2d(obj[0][0], dPoint(lon, lat))>10) return;
//  if (obj.name == name) obj.name = repl;
  obj.name = std::regex_replace(obj.name, std::regex(s), r);
}

// custom filter for vmaps
void filter_vmap(VMap2 & vmap){

  // Create labels
  vmap.iter_start();
  while (!vmap.iter_end()){
    auto p = vmap.iter_get_next();
    auto id = p.first;
    auto & obj = p.second;

    // all types
    change_name(obj, -1, NAN,NAN, u8"Ylimmäi(nen|set|sen) ?", u8"Yl.");
    change_name(obj, -1, NAN,NAN, u8"Keskimmäi(nen|set|sen) ?", u8"Kesk.");
    change_name(obj, -1, NAN,NAN, u8"Alimmai(nen|set|sen) ?", u8"Al.");
    change_name(obj, -1, NAN,NAN, u8"Alempi ", u8"Al.");
    change_name(obj, -1, NAN,NAN, u8"Ylempi ", u8"Yl.");

    change_name(obj, 0x2b04, NAN, NAN, u8"^koulu$", u8"");
    change_name(obj, 0x2b04, NAN, NAN, u8"^kappeli$", u8"");
    change_name(obj, 0x2b04, NAN, NAN, u8"^kirkko$", u8"");
    change_name(obj, 0x2b04, NAN, NAN, u8"^terveyskeskus$", u8"");
    change_name(obj, 0x2b04, NAN, NAN, u8"^b$", u8""); // Polttoaineen jakelupiste
    change_name(obj, 0x2b04, NAN, NAN, u8"^p$", u8""); // Posti
    change_name(obj, 0x2b04, NAN, NAN, u8"^h.as$", u8"");

    change_name(obj, 0x2b04, NAN, NAN, u8"^k.talo", u8""); // kaupungintalo
    change_name(obj, 0x2b04, NAN, NAN, u8"^s.talo", u8""); // seurantalo
//    change_name(obj, 0x2b04, NAN, NAN, u8"^kpa$", u8""); // Kauppa
//    change_name(obj, 0x2b04, NAN, NAN, u8"^leir.alue", u8""); // camping
    change_name(obj, 0x2b04, NAN, NAN, u8"^vanhaink.", u8""); // д/о

    // V51

    change_name(obj, 0x2b04, 28.021351, 68.209214,
      u8"^Kuuselankämppä\\\\vuokratupa", u8"vuokratupa");

    change_name(obj, 0x2b04, 28.026177, 68.210752,
      u8"^Rajankämppä\\\\autiotupa", u8" ");

    change_name(obj, 0x2b04, 28.258217, 68.258433,
      u8"^Muorravaarakanruoktu\\\\varauskammi", u8"varauskammi");

    // V52
    change_name(obj, 0x2800, 27.377093, 68.329614,
      u8"^Tievatupa", "");

    change_name(obj, 0x2800, 28.294852, 68.433827,
      u8"^Suomun Ville", "");

    change_name(obj, 0x6606, 27.442198, 68.434226,
      u8"^kolmiomittauksen muistomerkki\\\\näkötorni", u8"näkötorni");

    change_name(obj, 0x2b04, 27.433983, 68.421877,
      u8"^Kelo-ojan kullankaivajan tupa\\\\päivätupa", " ");

    change_name(obj, 0x2b04, 27.433026, 68.421303,
      u8"^Karvaselän kummituskämppä\\\\päivätupa", u8"päivätupa");

    change_name(obj, 0x2b04, 27.424318, 68.394476,
      u8"^Prospektorin kaivoskämppä\\\\päivätupa", u8"päivätupa");

    change_name(obj, 0x2800, 27.431137, 68.410316,
      u8"^Hirvaspirtti", "");

    change_name(obj, 0x2800, 27.504833, 68.602821,
      u8"^luontopolku", "");

    change_name(obj, 0x2800, 27.523532, 68.654741,
      u8"^moottorikelkkareitti", "");

    if (obj.name == "") vmap.del(id);
    else vmap.put(id, obj);
  }
}

/**********************************************************/

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

    filter_vmap(vmap1);

    // attach labels, get ref_id -> text_id multimap
    auto refs1 = vmap1.find_refs(100,200);
    auto refs2 = vmap2.find_refs(100,200);

    // Check all source objects
    auto i=refs1.begin();
    while (i!=refs1.end()){
      auto id = i->first;
      if (id == 0xFFFFFFFF){ // unconnected label
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }
      auto o1 = vmap1.get(id); // ref point

      // work only with D1/D2 objects
      if (o1.tags.count("D1")==0 && o1.tags.count("D2")==0) {
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }

      // find point with same type and npts, nearest coordinates, and tag D1/D2 in vmap2
      dRect rng = o1.bbox(); rng.expand(2e-4);
      uint32_t minid = -1;
      double mindist = INFINITY;
      for (const auto & i2: vmap2.find(o1.type, rng)){
        auto o2 = vmap2.get(i2);
        if (o2.tags.count("D1")==0 && o2.tags.count("D2")==0) continue;
        double d = dist(o1,o2); // inf for different npts, nsegments
        if (d<mindist){ mindist = d; minid = i2;}
      }
      // object found
      if (mindist<1e-4){
        auto i2 = minid;
        auto o2 = vmap2.get(i2);

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

        // go to next object in vmap1
        while (i!=refs1.end() && i->first == id) ++i;
        continue;
      }

      // if object is missing in vmap2, transfer it with all labels
      vmap2.add(o1);
      std::cout << "  add object: " << VMap2obj::print_type(o1.type) << ": " << o1.name << " " << o1[0][0] << "\n";

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

      if (id == 0xFFFFFFFF){ // unconnected label
        while (j!=refs1.end() && j->first == id) ++j;
        continue;
      }

      auto o2 = vmap2.get(id); // ref point
      if (o2.tags.count("D1")==0 && o2.tags.count("D2")==0) {
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // Find nearest vmap1 object with same type
      uint32_t minid = -1;
      double mindist = INFINITY;
      dRect rng = o2.bbox(); rng.expand(2e-4);
      for (const auto & i1: vmap1.find(o2.type, rng)){
        auto o1 = vmap1.get(i1);
        if (o1.tags.count("D1")==0 && o1.tags.count("D2")==0) continue;
        double d = dist(o1,o2); // inf for different npts, nsegments
        if (d<mindist){ mindist = d; minid = i1;}
      }

      // if object found, skip destination object
      if (mindist<1e-4){
        while (j!=refs2.end() && j->first == id) ++j;
        continue;
      }

      // if not, remove destination object and all labels
      std::cout << "  del object: " << VMap2obj::print_type(o2.type) << ": " << o2.name << " " << o2[0][0] << "\n";
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
