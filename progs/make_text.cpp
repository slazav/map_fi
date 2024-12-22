#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shp.h"
#include "shape/dbf.h"
#include "geo_data/conv_geo.h"
#include "geo_data/geo_utils.h"
#include "tmpdir/tmpdir.h"
#include "geom/poly_tools.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"
#include "vmap2/vmap2tools.h"

using namespace std;

// Read original zip archive <name>.zip with shape files,
// create text vmap (text + ref objects).

// We want to get rid of all duplicated refpoints:
// - remove second/third languages (this can be done using text metadata)
// - merge multi-line text
// - attach multiple labels (along long river) to a single point
//   (if you want to use label updating lates, you should set label_maxnum
//   for some reference types in types.conf!)

// Type of referece object for text
std::map<int, std::string> ref_types = {
 {1010210, "point:0x1100"},  // summit (vaara)
 {1010205, "point:0x0D00"},  // group of summits (vaarat)

 {2010210, "point:0x650C"},  // lake (jarvi)
 {2010215, "point:0x650C"},  // big lake?
 {2010205, "point:0x650C"},  // group of lakes (lammit, jarvet)
 {2010220, "point:0x650C"},  // reservoir
 {2010230, "point:0x650C"},  // canal
 {2020105, "point:0x650C"},  // river
 {2020110, "point:0x650C"},  // bay
 {2020115, "point:0x6508"},  // rapids

 {3010105, "point:0x700"},  // towns
 {3010110, "point:0x800"},  // villages
 {3020105, "point:0x900"},  // small villages
 {3030105, "point:0x900"},  // small villages ?

 {1010105, "point:0x2800"},  // groups of islands (saaret)
 {1010110, "point:0x2800"},  // island (saari)
 {1010120, "point:0x2800"},  // peninsula (niemi)
 {1020105, "point:0x2800"},  // place
 {1020205, "point:0x2800"},  // swamp?
 {1010305, "point:0x2800"},  // kanyon (kuru)

 {5010105, "point:0x2800"},  // national parks
 {5010110, "point:0x2800"},  // parks
 {5999905, "point:0x2800"},  // protected areas
 {6010405, "point:0x2800"},  // airoport

 {8030130, "point:0x2800"},  // house ?
 {8030150, "point:0x2800"},  // house ?
 {8030199, "point:0x2800"},  // 
 {8030120, "point:0x2800"},  // 

 {1999905, "point:0x2800"},  // ?
};

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <in_file.zip> <out_file.vmap2>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD","VMAP2"});
  throw Err();
}

// parse <x>;<y> string
dPoint
parse_crd(const std::string & s){
  size_t n = s.find(";");
  if (n==string::npos) return dPoint();
  auto a = s.substr(0,n);
  auto b = s.substr(n+1,s.size());
  if (a.size()==0 || b.size()==0) return dPoint();
  return dPoint(str_to_type<double>(a), str_to_type<double>(b));
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

    VMap2 vmap;
    ConvGeo cnv("ETRS-TM35FIN");

    // unzip data to a temporary dir
    std::cerr << "reading data: " << in_file << "\n";
    TmpDir tmp_dir("shp2vmap_XXXXXX");
    tmp_dir.unzip(in_file);
    std::string map_name = file_get_basename(in_file);

    // open shape databases
    std::string base = tmp_dir.get_dir() + "/" + map_name + "_KarttanimiPiste";
    std::string shp = base + ".shp";
    std::string dbf = base + ".dbf";
    if (!file_exists(shp))
      throw Err() << "no SHP file: " << base;
    if (!file_exists(dbf))
      throw Err() << "no DBF file: " << base;
    Shp SH(shp.c_str(), 0);
    Dbf DB(dbf.c_str(), 0);

    int nr = DB.nrecords();
    int nf = DB.nfields();

    if (SH.get_num() != nr)
      throw Err() << "different number of objects in SHP and DBF: " << base;


    // Pass1: go through all objects, collect them to txt_store.
    // Detect and merge/mark duplicates, remove unwanted
    std::vector<Opt> txt_store;
    std::set<int> multi_types; // ref types with multiple objects
    for (size_t i = 0; i<nr; ++i){

      // collect object info to obj
      Opt o;
      for (size_t j = 0; j<nf; ++j)
        o.put<std::string>(DB.field_name(j), DB.get_str(i,j));

      // skip objects with empty or missing text field
      std::string name1 = o.get("text");
      if (name1 == "") continue;

      // skip second language
      if (o.get<int>("languagedo") > 1) continue;

      // text coordinates
      dPoint crd1 = parse_crd(o.get<std::string>("textpositi"));
      // is it same as coordinate from shape?
      auto l = SH.get(i);
      if (l.npts()<1 || dist2d(l[0][0], crd1)>1)
        std::cerr << "different coords in textpositi and shp: "
                  << crd1 << " vs " << l << "\n";


      // Reference type.
      // all types should be in the tables above
      int pt = o.get<int>("placetype");
      if (ref_types.count(pt)==0){
        std::cerr << "skipping unknown text place type: " << pt << ": " << name1;
        continue;
      }
      auto ref_type1 = ref_types[pt];
      o.put("ref_type", ref_type1);

      // Reference point
      dPoint ref_pt1 = parse_crd(o.get<std::string>("placelocat"));

      // Check duplicates (same ref_type, same ref_pt)
      double ref_acc = 10; // m
      for (auto & o2: txt_store){
        std::string ref_type2 = o2.get("ref_type");
        if (ref_type2 != ref_type1) continue;

        dPoint ref_pt2 = parse_crd(o2.get<std::string>("placelocat"));
        if (dist2d(ref_pt1, ref_pt2) > ref_acc) continue;

        // We found point with same reference.
        // If names are different, merge them (upper label goes first).
        std::string name2 = o2.get("text");
        if (name1!=name2){
          dPoint crd2 = parse_crd(o.get<std::string>("textpositi"));
          // upper label -> o2 and name1
          if (crd2.y>crd1.y) name1.swap(name2);
          else o.swap(o2);
          if (name1.size()>0){
            if (name1[name1.size()-1]=='-') name1.resize(name1.size()-1);
            else name1+=' ';
          }
          o2.put("text", name1 + name2);
          name1 = ""; // we want to skip this text
        }
        // If names are same mark this text as duplicate.
        // (many names on a long river)
        else {
          o.put("duplicate", "1");
        }
      }
      if (name1=="") continue;

      txt_store.push_back(o);
    }

    // Pass2: convert to vmap
    for (const auto & o: txt_store){

      // Text object:
      VMap2obj objt("text:1");
      objt.add_point(parse_crd(o.get<std::string>("textpositi")));
      objt.ref_pt = parse_crd(o.get<std::string>("placelocat"));
      objt.set_ref_type(o.get("ref_type"));
      objt.name = o.get("text");

      // shorten name
      auto n = objt.name.find(" eli ");
      if (n != std::string::npos)
        objt.name = objt.name.substr(0,n);

      // modify text parameters for some place types
      int pt = o.get<int>("placetype");

      // align:center for rivers, etc.
      //if (pt == 1010205 || pt == 2020105 || pt == 1010305 || pt == 2010230)
      //  objt.align = VMAP2_ALIGN_S;

      // preserve text scaling
      objt.scale = o.get<int>("textsize")/200.0;

      // fixed scale for summits and summit groups
      if (pt == 1010210 || pt == 1010210)
        objt.scale = 1.0;

      // fixed small scale for some objects (rapids, canyons, houses)
      if (pt == 2020115 || pt == 1010305 || pt > 8000000)
        objt.scale = 0.75;

      // attach altitude to summit names
      if (pt == 1010210)
        objt.name = o.get<std::string>("placeeleva") + ' ' + objt.name;

      // parse text angle
      dPoint dir = parse_crd(o.get<std::string>("textdirect"));
      if (dir != dPoint())
        objt.angle = - 180/M_PI * atan2(dir.y - objt[0][0].y, dir.x - objt[0][0].x);


      cnv.frw(objt);
      cnv.frw(objt.ref_pt);
      vmap.add(objt);
      // Ref object:
      if (o.get<std::string>("duplicate") == "1"){
        multi_types.insert(objt.ref_type);
        continue;
      }
      VMap2obj objr;
      objr.add_point(objt.ref_pt);
      objr.type = objt.ref_type;
      objr.name = objt.name;
      vmap.add(objr);
    }

    // read file with type information if it's available
    VMap2types types(O);
    // allow multiple labels for reference objects
    for (const auto & t:multi_types){
      std::cerr << "reference type with duplicated labels: "
                << VMap2obj::print_type(t) << "\n";
      types[t].label_maxnum = -1;
    }

    do_update_labels(vmap, types);
    vmap2_export(vmap, types, out_file, Opt());
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

