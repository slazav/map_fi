#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_data/conv_geo.h"
#include "geo_data/geo_utils.h"
#include "geom/poly_tools.h"
#include <regex>
#include "lib.h"

using namespace std;

// Read original data (in vmap2 form) and produce F1 base map

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "make_map");
  pr.name("make base + add map");
  pr.usage("[<options>] <name> <data_dir>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}

/************************************************/
void
import_fi1(VMap2 & vmap, VMap2 & vmapt, VMap2 & vmapx, const std::string & name, const std::string & data_dir){

  // read type conversion table
  auto typ_convs = read_tconv_str("types_fi1.txt");
  auto txt_convs = read_tconv_txt("types_fi1t.txt");

  // preferred languages:
  std::vector<std::string> langpref = {"fin", "sme", "smn", "sms", "swe"};

  VMap2 vmap_src;
  read_src(vmap_src, data_dir + "/" + name + "_fi1.vmap2.gz");
  if (vmap_src.size()==0) return;

  // we want to sort text objects by placeid field, then by -y coordinate
  std::map<int, std::multimap<double, VMap2obj> > text_buf;

  vmap_src.iter_start();
  while (!vmap_src.iter_end()){
    auto pi = vmap_src.iter_get_next();
    auto ii = pi.first;
    auto oi = pi.second;

    /**********************************/
    // text types
    if (oi.is_class(VMAP2_TEXT)) {
      // Find type info:
      tconv_txt_t tinfo = {"point:34600", "text:1", 1, "0.75"}; // default
      auto t = oi.opts.get("placetype");
      if (txt_convs.count(t)!=0)
        tinfo = txt_convs[t];
      else {
        std::cerr << "Unknown text type: " << t << " "
                  << oi.get_first_pt() << " " << oi.opts.get("spelling") << "\n";
        continue;
      }

      VMap2obj obj(tinfo.type);
      obj.add_point(oi.get_first_pt());
      obj.set_ref_type(tinfo.rtype);
      obj.ref_pt = oi.ref_pt;
      // Always use "spelling" field: we can have double labels,
      // but without "or" in different languages and without carry-overs
      obj.name = oi.opts.get("spelling");

      // for summits attach altitude
      if (tinfo.rtype == "point:0x1100" && oi.opts.exists("placeeleva"))
        obj.name = oi.opts.get("placeeleva") + " " + obj.name;

      if (tinfo.angle) obj.angle = oi.angle;
      if (tinfo.scale.size() && tinfo.scale[0]=='x'){
        obj.scale = oi.scale*str_to_type<double>(tinfo.scale.substr(1));
        if (obj.scale<0.75) obj.scale = 0.75;
      }
      else obj.scale = str_to_type<double>(tinfo.scale);

      obj.opts.put("lang", oi.opts.get("language", ""));
      text_buf[oi.opts.get<int>("placeid")].emplace(-obj.get_first_pt().y, obj);
    }
    /**********************************/
    // other types
    else {
      // convert type
      auto t = oi.opts.get("type1");
      if (typ_convs.count(t)==0){
        std::cerr << "unknown type: " << t << " " << oi.get_first_pt() << "\n";
        continue;
      }
      if (typ_convs[t]=="-") continue;
      t = typ_convs[t];

      // create object and set coordinates
      VMap2obj obj(t);
      obj.dMultiLine::operator=(oi);

      // contours names (D1)
      if (t == "line:0x21" && oi.opts.exists("height")){
        double k = oi.opts.get<double>("height");
        obj.name = oi.opts.get("height");
        if (int(k)%50 == 0) obj.set_type("line:0x22");
      }

      obj.opts.put("type1",  oi.opts.get("type1"));
      obj.opts.put("type2",  oi.opts.get("type2"));
      obj.opts.put("Source", "FI1");
      vmap.add(obj);
    }
  }

  /**********************************/
  // Second pass for text objects

  for (const auto tt: text_buf){
    // one object can contain a few labels:
    // - multiple languages - we want to keep most preferred one
    // - carry-over (text is same because we used "spelling" field), we want to keep upper one
    // - a few labels for a long river - we want to keep all

    // Find preferred language:
    auto ln = langpref.size();
    for (const auto o: tt.second){
      for (int i = 0; i<ln; ++i){
        if (o.second.opts.get("lang") != langpref[i]) continue;
        ln = i; break;
      }
    }
    if (ln == langpref.size()) throw Err()
       << "can't find preferred language: " << tt.second.begin()->second;

    bool first = true;
    dPoint pt; // previous point
    for (const auto o: tt.second){
      // put summits to extra map
      bool extra = o.second.is_ref_type("point:0x1100");

      if (first) {
        // make reference object
        auto oref = make_ref_obj(o.second, "FI1");
        if (extra) vmapx.add(oref);
        else       vmapt.add(oref);
      }
      // skip other languages
      if (o.second.opts.get("lang") != langpref[ln]) continue;

      // keep first label and labels far from it
      if (first || geo_dist_2d(pt, o.second.get_first_pt()) > 1000){
         if (extra) vmapx.add(o.second);
         else vmapt.add(o.second);
      }
      pt = o.second.get_first_pt();
      first=false;
    }
  }

}

/************************************************/

void
import_fi2(VMap2 & vmap, VMap2 & vmapt, VMap2 & vmapx, const std::string & name, const std::string & data_dir){

  // read type conversion table
  auto typ_convs = read_tconv_fi2("types_fi2.txt");
  auto txt_convs = read_tconv_fi2t("types_fi2t.txt");

  VMap2 vmap_src;
  read_src(vmap_src, data_dir + "/" + name + "_fi2.vmap2.gz");
  if (vmap_src.size()==0) return;

  // remove types from FI1 map
  std::set<uint32_t> toremove;
  vmap.iter_start();
  while (!vmap.iter_end()){
    auto pi = vmap.iter_get_next();
    auto t = pi.second.opts.get("type1");
    if (typ_convs.count(t)==0) continue;
    auto act = typ_convs[t].action;
    if (act == "R" || act == "D") toremove.insert(pi.first);
  }
  for (const auto & i: toremove) vmap.del(i);

  // group labels by reference points, sort by -y coordinate
  std::list<std::multimap<double, VMap2obj> > label_groups;

  /********************************************************/

  vmap_src.iter_start();
  while (!vmap_src.iter_end()){
    auto pi = vmap_src.iter_get_next();
    auto ii = pi.first;
    auto oi = pi.second;

    /**********************************/
    // text objects
    if (oi.is_class(VMAP2_TEXT)) {
      // Find type info:
      tconv_fi2t_t tinfo = {"point:34600", "text:1", 0, 1, "0.75"}; // default
      std::string t = oi.opts.get("type1").substr(5); // remove text: prefix
      if (txt_convs.count(t)!=0)
        tinfo = txt_convs[t];
      else {
        std::cerr << "Unknown text type: " << t << " "
                  << oi.get_first_pt() << " " << oi.opts.get("TEKSTI") << "\n";
        continue;
      }
      if (tinfo.type == "-" || tinfo.rtype == "") continue;

      VMap2obj obj(tinfo.type);
      obj.add_point(oi.get_first_pt());
      obj.set_ref_type(tinfo.rtype);
      obj.ref_pt = oi.ref_pt;
      obj.name = oi.opts.get("TEKSTI");
      if (obj.name == "") continue;

      if (tinfo.legend){
        obj.name[0] = tolower(obj.name[0]);
        obj.opts.put("legend", "1");
      }

      if (tinfo.angle) obj.angle = oi.angle;
      if (tinfo.scale.size() && tinfo.scale[0]=='x'){
        obj.scale = oi.scale*str_to_type<double>(tinfo.scale.substr(1));
        if (obj.scale<0.75) obj.scale = 0.75;
      }
      else obj.scale = str_to_type<double>(tinfo.scale);

      // water altitude mark  -> directly to extra map
      if (t == "36291"){
        // name could be 123.40 or (243.50-245.00)
        obj.name = oi.opts.get("TEKSTI");
        // remove fractional part
        obj.name = std::regex_replace(obj.name, std::regex("\\.[0-9]+"), "");
        vmapx.add(obj);
        vmapx.add(make_ref_obj(obj, "FI2"));
        continue;
      }

      // We want to combine together text objects with
      // same ref_type (not input type), close ref_pt
      // Select correct label:
      // - language preference -- not supported in FI2
      // + upper label first, skip lower (same or different names)
      // + join upper legend

      bool found = false;
      for (auto & group: label_groups){
        for (const auto & i: group){
          auto & obj2 = i.second;
          if (obj.ref_type != obj2.ref_type) continue;
          if (geo_dist_2d(obj.ref_pt, obj2.ref_pt) > 50) continue;
          found = true;
          break;
        }
        if (found){
          group.emplace(-obj.get_first_pt().y, obj);
          break;
        }
      }
      if (!found) {
        std::multimap<double, VMap2obj> group;
        group.emplace(-obj.get_first_pt().y, obj);
        label_groups.push_back(group);
      }
    }
    /**********************************/
    // other objects
    else {
      // check type
      std::string ti = oi.opts.get("type1");
      if (typ_convs.count(ti)==0){
        std::cerr << "unknown type: " << ti << " " << oi.get_first_pt() << "\n";
        continue;
      }
      auto act = typ_convs[ti].action;
      if (act=="-" || act=="D") continue;
      if (act!="A" && act!="R")
        throw Err() << "Unknown action in type conversion file: " << act;

      auto t = typ_convs[ti].type;

      // create object and set coordinates
      VMap2obj obj(t);
      obj.dMultiLine::operator=(oi);

      // contours names
      if (t == "line:0x21" && oi.opts.exists("height")){
        double k = oi.opts.get<double>("height");
        obj.name = oi.opts.get("height");
        if (int(k)%50 == 0) obj.set_type("line:0x22");
      }

      // for trails (but not pedestrian roads!) treat Paallyste/PAALLY = 2 as a "bridge"
      // create additional object.
        if (oi.opts.get<int>("sidewalk") > 1 && ti == "line:12313"){
        VMap2obj obj1("line:0x08");
        obj1.dMultiLine::operator=(oi);
        obj1.opts.put("Source", "FI2");
        vmap.add(obj1);
      }

      obj.opts.put("type1", oi.opts.get("type1"));
      obj.opts.put("type2", oi.opts.get("type2"));
      obj.opts.put("Source", "FI2");
      vmap.add(obj);
    }
  }

  /**********************************/
  // Second pass for text objects
  // Example of complicated groups:
  //   Name(lang1) Name(lang2) Leg Name(lang1) Name(lang2) Leg [68.25827 28.25834]  (two objects are merged)
  //   Leg1 Leg2 [68.43527 27.44415]
  //   Name(lang1), Name(lang2), far from each other, single refpoint: 21.321321,69.129069
  // I fill only names which go after another name.
  // Here we have strictly one ref_pt and one label for each group.
  // In FI1 and NO1 we may want to keep far labels with same text (rivers)

  for (auto group: label_groups){
    // append names and legends to the first object
    if (group.size()==0) continue;
    auto & obj = group.begin()->second;

    // TODO: river labels
//    for (auto i = group.begin(); i!=group.end(); ++i);
//      if (i==group.begin()) continue;
//      if (i->second.name == obj.name) &&
//         (geo_dist_2d(i->second.get_first_pt, obj.get_first_pt) > 1000)
//        vmapt.add(i->second);
//    }

    bool lp = false;
    for (auto i = group.begin(); i!=group.end();){
      bool l = i->second.opts.exists("legend");
      if (i==group.begin()) { lp=l; ++i; continue;}
      if (!l && !lp){ i = group.erase(i); continue; }
      obj.name += "\\" + i->second.name;
      lp=l; ++i;
    }


    vmapt.add(obj);
    vmapt.add(make_ref_obj(obj, "FI2"));
  }
}

/************************************************/

void
import_no1(VMap2 & vmap, VMap2 & vmapt, VMap2 & vmapx, const std::string & name, const std::string & data_dir){

  auto typ_convs = read_tconv_str("types_no1.txt");
  auto txt_convs = read_tconv_txt("types_no1t.txt");

  // read source data
  VMap2 vmap_src;
  read_src(vmap_src, data_dir + "/" + name + "_no1.vmap2.gz");
  if (vmap_src.size()==0) return;

  // We want to sort text objects by place_number field, then by -y coordinate
  // Similar to fi1t (unfortunately all languages are always set to "nor")
  std::map<std::string, std::multimap<double, VMap2obj> > text_buf;

  vmap_src.iter_start();
  while (!vmap_src.iter_end()){
    auto pi = vmap_src.iter_get_next();
    auto ii = pi.first;
    auto oi = pi.second;

    /********************************/
    // text objects
    // There are two different text types:
    // "place_name_text", imported as "text:1"
    // and "presentation_text" imported as "text:2"
    if (oi.is_type("text:1")){

      std::string t = oi.opts.get("text_type");

      // types
      tconv_txt_t tinfo = {"point:34600", "text:1", 1, "1.00"}; // default
      if (txt_convs.count(t)!=0)
        tinfo = txt_convs[t];
      else {
        std::cerr << "Unknown text type: "
                  << oi.opts.get("text_fulltype", t)
                  << "\n  " << oi.get_first_pt() << "\n";
        // use default
      }
      if (tinfo.type=="-") continue;

      VMap2obj obj;
      obj.set_type(tinfo.type);
      obj.set_ref_type(tinfo.rtype);


      obj.name = oi.opts.get("fulltext");

      // alignment
      int ha = oi.opts.get<int>("horizontalalignment"); // 0,1,2
      obj.align=VMAP2_ALIGN_SW;
      if (ha == 1) obj.align=VMAP2_ALIGN_S;
      if (ha == 2) obj.align=VMAP2_ALIGN_SE;

      // angle
      obj.angle = -oi.opts.get<double>("angle");
      if (!tinfo.angle) obj.angle=NAN;

      // scale
      obj.scale = oi.opts.get<double>("fontsize")/9.0;
      if (tinfo.scale.size() && tinfo.scale[0]=='x'){
        obj.scale *= str_to_type<double>(tinfo.scale.substr(1));
        if (obj.scale<0.75) obj.scale = 0.75;
      }
      else obj.scale = str_to_type<double>(tinfo.scale);

      obj.add_point(oi.get_first_pt());
      obj.ref_pt = oi.get_first_pt();

      // place_number or point
      text_buf[oi.opts.get("place_number")].emplace(-obj.get_first_pt().y, obj);
    }
    /********************************/
    else if (oi.is_type("text:2")){

      std::string t = oi.opts.get("text_type");

      VMap2obj obj;
      obj.name = oi.opts.get("fulltext");
      obj.add_point(oi.get_first_pt());
      obj.ref_pt = oi.get_first_pt(); // no good ref_point
 
      // water altitude marks (exact positions are not important) -> to extra map
      if (t == "hoydetallVann") {
        obj.set_type("text:4");
        obj.set_ref_type("point:0x1000");
        obj.scale = 0.75;
        vmapx.add(obj);
        vmapx.add(make_ref_obj(obj, "NO1"));
        continue;
      }
      // trigs - will be taken from point objects
      else if (t == "fastmerke"){
        continue;
      }
      // glacier altitude marks - skip them because we do not have exact positions
      else if (t == "hoydetallPunktIsbre"){
        continue;
      }
      // altitude marks - will be taken from point objects
      else if (t == "hoydetallPunkt"){
        continue;
      }
      else {
        std::cerr << "unknown presentation_text type: " << t << "\n";
        continue;
      }
    }
    /********************************/
    // other objects
    else {
      std::string t = oi.opts.get("table");
      // check type
      if (typ_convs.count(t)==0){
        std::cerr << "unknown type: " << t << " " << oi.get_first_pt() << "\n";
        continue;
      }
      if (typ_convs[t] == "-") continue;
      t = typ_convs[t];

      // create object and set coordinates
      VMap2obj obj;
      obj.dMultiLine::operator=(oi);
      obj.name = oi.opts.get("name");
      obj.opts.put("Source", "NO1");

      // special type contours
      if (t == "cnt"){
        obj.name = oi.opts.get("height");
        double k = oi.opts.get<double>("height");
        if (int(k)%100 == 0) t = "line:0x22";
        else  t = "line:0x21";
      }

      // special types: rivers
      if (t == "river"){
        int ww = oi.opts.get<int>("water_width");
        switch (ww){
          case 1: t="line:0x26"; break;
          case 2: t="line:0x15"; break;
          case 3: t="line:0x18"; break;
          case 4: t="line:0x1F"; break;
          default: {std::cerr << "unknown water_width: " << ww << "\n"; continue; }
        }
      }

      // special types: roads
      if (t == "road"){
        auto rt = oi.opts.get("type_road");
        if (rt == "enkelBilveg")          t = "line:0x02"; // simple road ?
        else if (rt == "traktorveg")      t = "line:0x0A"; // mud road
        else if (rt == "sti")             t = "line:0x16";    // path
        else if (rt == "barmarksloype")   t = "line:0x2A";    // foot trail
        else if (rt == "gangOgSykkelveg") t = "line:0x2A";    // walking-or-cycling
        else { std::cout << "unknown type_road: " << rt << "\n"; continue;}
      }

      // Moving to label/extra map:

      // special types: hytta, with and without name -> to text map
      if (oi.opts.get("table") == "building_position" &&
          oi.opts.get("building_type") == "956") {

        obj.set_type("point:0x2B04");
        vmapt.add(obj);

        if (obj.name == "") continue;
        VMap2obj txt("text:3");
        txt.scale = 0.75;
        txt.name = obj.name;
        txt.ref_pt = obj.get_first_pt();
        txt.ref_type = obj.type;
        txt.add_point(obj.get_first_pt() + dPoint(3e-3,0));
        vmapt.add(txt);
        continue;
      }

      // other buildings with names -> to text map
      if (oi.opts.get("table") == "building_position" &&
          obj.name!="") {

        vmapt.add(obj);

        VMap2obj txt("text:3");
        txt.scale = 0.75;
        txt.name = obj.name;
        txt.ref_pt = obj.get_first_pt();
        txt.ref_type = obj.type;
        txt.add_point(obj.get_first_pt() + dPoint(3e-3,0));
        vmapt.add(txt);
        continue;
      }

      // special types: trig -> to text map
      if (oi.opts.get("table") == "trigonometric_point" &&
          oi.opts.get("height") != "") {

        obj.set_type("point:0x0F00");
        obj.name = oi.opts.get("height");
        vmapt.add(obj);

        VMap2obj txt("text:6");
        txt.name = obj.name;
        txt.scale = 0.8;
        txt.ref_pt = obj.get_first_pt();
        txt.ref_type = obj.type;
        txt.add_point(obj.get_first_pt() + dPoint(3e-3,0));
        vmapt.add(txt);
        continue;
      }

      // altitude points -> to extra map
      if (oi.opts.get("table") == "terrain_point" &&
          oi.opts.get("height") != "") {

        obj.set_type("point:0x0D00");
        obj.name = oi.opts.get("height");
        vmapx.add(obj);

        VMap2obj txt("text:6");
        txt.name = obj.name;
        txt.scale = 0.8;
        txt.ref_pt = obj.get_first_pt();
        txt.ref_type = obj.type;
        txt.add_point(obj.get_first_pt() + dPoint(3e-3,0));
        vmapx.add(txt);
        continue;
      }

      obj.set_type(t);
      vmap.add(obj);
    }
  }

  /********************************/
  // Second pass for text objects
  for (const auto tt: text_buf){
    // Just keep the upper label and labels with same name far from original

    bool first = true;
    dPoint pt; // main point
    std::string nm; // main name
    for (auto o: tt.second){

      // to extra map
      bool ex = o.second.is_ref_type("point:0x1100");
      VMap2 & map = ex? vmapx : vmapt;

      if (first) {

        // Check that same name does not appear near it
        // This is needed because some buiding names could be
        // converted from non-text objects.
        if (map.find_nearest(o.second.type, o.second.name, o.second.ref_pt, 1000)!=-1)
          continue;

        // make reference object
        map.add(make_ref_obj(o.second, "NO1"));

        // save object
        map.add(o.second);

        pt = o.second.get_first_pt();
        nm = o.second.name;
        first=false;
        continue;
      }
      // keep other labels with same name which are far enough
      if (geo_dist_2d(pt, o.second.get_first_pt()) > 1000 &&
          nm == o.second.name) map.add(o.second);
    }
  }
}

/************************************************/

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 2) usage();
    std::string name = args[0];
    std::string data_dir = args[1];

    VMap2 vmap, vmapt, vmapx;
    import_fi1(vmap, vmapt, vmapx, name, data_dir);
    import_fi2(vmap, vmapt, vmapx, name, data_dir);
    import_no1(vmap, vmapt, vmapx, name, data_dir);

    do_join_lines(vmap, 20, 30);
    vmap2_export(vmap,  VMap2types(), name + "_b.vmap2.gz", Opt());
    vmap2_export(vmapt, VMap2types(), name + "_t.vmap2.gz", Opt());
    vmap2_export(vmapx, VMap2types(), name + "_x.vmap2.gz", Opt());

  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
