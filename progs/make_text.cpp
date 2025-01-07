#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shape_db.h"
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
struct ref_type_t{
  std::string type;
  bool angle; // true to keep original, false to unset
  double scale; // NAN to keep original
  bool legend; // is it legend (selite) or name (nimi)?
};

std::map<int, ref_type_t> ref_types = {
// D1 types
 {1010210, {"point:0x1100",   0, 1.0, 0}},  // summit (vaara)
 {1010205, {"point:0x0D00",   0, 1.0, 0}},  // group of summits (vaarat)

 {2010210, {"point:0x650C",   0, NAN, 0}},  // lake (jarvi)
 {2010215, {"point:0x650C",   0, NAN, 0}},  // big lake?
 {2010205, {"point:0x650C",   0, NAN, 0}},  // group of lakes (lammit, jarvet)
 {2010220, {"point:0x650C",   0, NAN, 0}},  // reservoir
 {2010230, {"point:0x650C",   1, NAN, 0}},  // canal
 {2020105, {"point:0x650C",   1, NAN, 0}},  // river
 {2020110, {"point:0x650C",   0, NAN, 0}},  // bay
 {2020115, {"point:0x6508",   0, 0.75, 0}},  // rapids

 {3010105, {"point:0x700",   0, NAN, 0}},  // towns
 {3010110, {"point:0x800",   0, NAN, 0}},  // villages
 {3020105, {"point:0x900",   0, NAN, 0}},  // small villages
 {3030105, {"point:0x900",   0, NAN, 0}},  // house
 {4010125, {"point:0x700",   0, NAN, 0}},  // big towns ?

 {1010105, {"point:0x2800",   0, NAN, 0}},  // groups of islands (saaret)
 {1010110, {"point:0x2800",   0, NAN, 0}},  // island (saari)
 {1010120, {"point:0x2800",   0, NAN, 0}},  // peninsula (niemi)
 {1010305, {"point:0x2800",   0, 0.75, 0}},  // kanyon (kuru)
 {1020105, {"point:0x2800",   0, NAN, 0}},  // place
 {1020205, {"point:0x2800",   0, NAN, 0}},  // swamp?
 {1020305, {"point:0x2800",   0, NAN, 0}},  // swamp?

 {1999905, {"point:0x2800",   0, NAN, 0}},  // ?

 {5010105, {"point:0x2800",   0, NAN, 0}},  // national parks
 {5010110, {"point:0x2800",   0, NAN, 0}},  // parks
 {5999905, {"point:0x2800",   0, NAN, 0}},  // protected areas
 {6010405, {"point:0x2800",   0, NAN, 0}},  // airoport

 {8030130, {"point:0x2800",   0, 0.75, 0}},  // house ?
 {8030150, {"point:0x2800",   0, 0.75, 0}},  // house ?
 {8030199, {"point:0x2800",   0, 0.75, 0}},  //
 {8030120, {"point:0x2800",   0, 0.75, 0}},  //

// detailed map
 {12106,   {"-", 0, 0.75, 0}}, //  Autotien lauttanumero
 {12181,   {"-", 0, 0.75, 0}}, //  Paikallistien numero
 {12182,   {"-", 0, 0.75, 0}}, //  Maantien numero
 {12183,   {"-", 0, 0.75, 0}}, //  E- valta- tai kantatien numero
 {12301,   {"point:0x2800", 1, 0.75, 0}}, //  Kulkuv.yl.n nimi / trail
 {12302,   {"point:0x2800", 1, 0.75, 1}}, //  Kulkuv.yl.n selite
 {14201,   {"-", 0, 0.75, 0}}, //  Rautatieliikennepaikan nimi
 {16101,   {"-", 0, 0.75, 0}}, //  Turvalaitteen nimi
 {16102,   {"-", 0, 0.75, 1}}, //  Turvalaitteen selite
 {16503,   {"-", 0, 0.75, 0}}, //  Kulkusyvyys (2.2mm teksti)
 {16504,   {"-", 0, 0.75, 0}}, //  Kulkusyvyys (1.8mm teksti)
 {16508,   {"-", 0, 0.75, 0}}, //  Alikulkukorkeus
 {16703,   {"-", 0, 0.75, 0}}, //  Hylyn syvyys / затонувшее судно, глубина
 {26202,   {"-", 0, 0.75, 1}}, //  Vedenottamon selite
 {32101,   {"-", 0, 0.75, 0}}, //  Maa-aineksenottoalueen nimi
 {32102,   {"-", 0, 0.75, 1}}, //  Maa-aineksenottoalueen selite
 {32201,   {"-", 0, 0.75, 0}}, //  Hautausmaan nimi
 {32202,   {"-", 0, 0.75, 1}}, //  Hautausmaan selite
 {32301,   {"-", 0, 0.75, 0}}, //  Kaatopaikan nimi
 {32302,   {"-", 0, 0.75, 1}}, //  Kaatopaikan selite
 {32401,   {"-", 0, 0.75, 0}}, //  Liikennealueen nimi
 {32402,   {"-", 0, 0.75, 1}}, //  Liikennealueen selite
 {32501,   {"-", 0, 0.75, 0}}, //  Louhoksen nimi / карьер
 {32502,   {"-", 0, 0.75, 1}}, //  Louhoksen selite / карьер
 {32602,   {"-", 0, 0.75, 1}}, //  Maatalousmaan selite
 {32901,   {"-", 0, 0.75, 0}}, //  Puiston nimi
 {32902,   {"-", 0, 0.75, 1}}, //  Puiston selite
 {33001,   {"-", 0, 0.75, 0}}, //  T.ytemaan nimi
 {33002,   {"-", 0, 0.75, 1}}, //  T.ytemaan selite
 {33101,   {"-", 0, 0.75, 0}}, //  Urheilu- ja virkistysalueen nimi
 {33102,   {"-", 0, 0.75, 1}}, //  Urheilu- ja virkistysalueen selite
 {34601,   {"-", 0, 0.75, 0}}, //  Kiven nimi
 {34901,   {"point:0x6601", 0, 0.75, 0}}, //  Merkitt.v.n luontokohteen nimi
 {34902,   {"point:0x6601", 0, 0.75, 1}}, //  Merkitt.v.n luontokohteen selite

 {35010,   {"-", 0, 0.75, 0}}, // field, meadow
 {35020,   {"-", 0, 0.75, 0}}, // forest
 {35030,   {"-", 0, 0.75, 0}}, // swamp
 {35040,   {"-", 0, 0.75, 0}}, // mountain
 {35050,   {"-", 0, 0.75, 0}}, // canyons
 {35060,   {"-", 0, 0.75, 0}}, // peninsula
 {35070,   {"-", 0, 0.75, 0}}, // island
 {35080,   {"-", 0, 0.75, 0}}, // shellow water
 {35090,   {"-", 0, 0.75, 0}}, // other land object

 {35101,   {"-", 0, 0.75, 0}}, // tree
 {35102,   {"-", 0, 0.75, 1}}, // tree - legend

 {36101,   {"-", 0, 0.75, 0}}, // bay
 {36201,   {"-", 0, 0.75, 0}}, // lake
 {36291,   {"-", 0, 0.75, 0}}, // water altitude mark (!)
 {36301,   {"-", 0, 0.75, 0}}, // river
 {36410,   {"-", 0, 0.75, 0}}, // part of lake
 {36420,   {"-", 0, 0.75, 0}}, // part of river
 {36490,   {"-", 0, 0.75, 0}}, // other water object
 {36500,   {"-", 0, 0.75, 1}}, // legend of other land object

 {38201,   {"-", 0, 0.75, 0}}, // rapids
 {38501,   {"-", 0, 0.75, 0}}, // water stone
 {38901,   {"-", 0, 0.75, 0}}, // storage area
 {38902,   {"-", 0, 0.75, 0}}, // storage area - legend
 {39502,   {"-", 0, 0.75, 0}}, // global forest border

 {42101,   {"point:0x2B04", 0, 0.75, 0}}, // building name (laavu, autiotupa)
 {42102,   {"point:0x2B04", 0, 0.75, 1}}, // bullding legend
 {42201,   {"-", 0, 0.75, 0}},  // group of buildings - name
 {42202,   {"-", 0, 0.75, 1}},  // group of buildings - legend
 {44202,   {"point:0x2800", 0, 0.75, 1}},  // fence - legend
 {44301,   {"-", 0, 0.75, 0}},  // water pool - name
 {44302,   {"-", 0, 0.75, 1}},  // water pool - legend
 {44402,   {"-", 0, 0.75, 1}},  // airline - legend
 {44803,   {"-", 0, 0.75, 0}}, // hight of mast
 {44901,   {"point:0x2C04", 0, 0.75, 0}}, // monument - name
 {44902,   {"point:0x2C04", 0, 0.75, 1}}, // monument - legend
 {45002,   {"point:0x6606", 0, 0.75, 1}}, // watch tower - legend
 {45303,   {"-", 0, 0.75, 0}}, // chimney hight
 {45402,   {"-", 0, 0.75, 1}}, // tar pit - legend
 {45502,   {"-", 0, 0.75, 1}}, // wind turbine - legend
 {45702,   {"-", 0, 0.75, 1}}, // structure - legend
 {45802,   {"-", 0, 0.75, 1}}, // water tower - legend

 {48111,   {"-", 0, 0.75, 0}}, // town
 {48112,   {"-", 0, 0.75, 0}}, // other community
 {48120,   {"-", 0, 0.75, 0}}, // village
 {48130,   {"-", 0, 0.75, 0}}, // house name
 {48190,   {"-", 0, 0.75, 0}}, // other settlement

 {52191,   {"-", 0, 0.75, 0}}, // contours
 {54191,   {"-", 0, 0.75, 0}}, // depth contours
 {54210,   {"-", 0, 0.75, 0}}, // depth marks
// {62102,   {"-", 0, 0.75, 0}}, // shooting range
 {62202,   {"-", 0, 0.75, 0}}, // protected area

 {72201,   {"-", 0, 0.75, 0}}, // protected nature area
 {72202,   {"-", 0, 0.75, 1}}, // protected nature area - legend
 {72303,   {"point:0x6601", 0, 0.75, 0}}, // natural monument (!)
 {72304,   {"point:0x6601", 0, 0.75, 1}}, // natural monument - legend
 {72403,   {"point:0x6415", 0, 1.0, 0}}, // archeological place
 {72404,   {"point:0x6415", 0, 1.0, 1}}, // archeological place - legend
 {72501,   {"-", 0, 0.75, 0}}, // nature park
 {72502,   {"-", 0, 0.75, 1}}, // nature park - legend
 {72601,   {"-", 0, 0.75, 0}}, // national park
 {72701,   {"-", 0, 0.75, 0}}, // wilderness area
 {72702,   {"-", 0, 0.75, 1}}, // wilderness area - legend
// {72801,   {"-", 0, 0.75, 0}}, // hiking area
// {72802,   {"-", 0, 0.75, 1}}, // hiking area - legend
 {82102,   {"-", 0, 0.75, 1}}, // Aluemeren ulkorajan selite
 {82202,   {"-", 0, 0.75, 1}}, // Rajavy.hykkeen takarajan selite / Finnish border zone
 {82302,   {"-", 0, 0.75, 1}}, // Sis.isten aluevesien ulkorajan selite
 {82402,   {"-", 0, 0.75, 1}}, // Ulko- ja sis.saariston rajan selite
 {82501,   {"-", 0, 0.75, 0}}, // Valtakunnan rajapyykin nimi
 {84300,   {"-", 0, 0.75, 0}}, // Kunnan hallintokeskus / муниципальный центр (ратуша и т.п.)
 {84301,   {"-", 0, 0.75, 0}}, // L..nin p..kaupunki / столица округа
 {84302,   {"-", 0, 0.75, 0}}, // Muu kaupunki / другой город
 {84303,   {"-", 0, 0.75, 0}}, // Muu kunta / другая область
 {85100,   {"-", 0, 0.75, 0}}, // Kunnan hallintorajan selite
 {92401,   {"-", 0, 0.75, 0}}, // Rajapyykin nimi
 {95302,   {"-", 0, 0.75, 1}}, // Vesiasteikon selite

};

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <src_dir> <name> <out_file>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD","VMAP2"});
  throw Err();
}

/************************************************/

// parse <x>;<y> string for [D1] text objects
dPoint
parse_crd(const std::string & s){
  size_t n = s.find(";");
  if (n==string::npos) return dPoint();
  auto a = s.substr(0,n);
  auto b = s.substr(n+1,s.size());
  if (a.size()==0 || b.size()==0) return dPoint();
  return dPoint(str_to_type<double>(a), str_to_type<double>(b));
}

// unzip all shape files for the map
void
unzip_all(TmpDir & tmp_dir, const std::string & data_dir, const std::string & name){
  auto f = data_dir + "/" + name + ".zip";
  std::cerr << "unzipping: " << f << "\n";
  tmp_dir.unzip(f);
  for (const auto & f: file_glob({data_dir + "/" + name + "???.shp.zip"})){
    std::cerr << "unzipping: " << f << "\n";
    tmp_dir.unzip(f);
  }
}

// detect data source from shape file name
int detect_src(const std::string & base){
  if (base.size()>3 && base[3]=='_') return 1;
  if (base.size()>1 && base[1]=='_') return 2;
  throw Err() << "unknown source type: " << base << "\n";
}

bool
check_suff(const std::string & str, const std::string & suff){
  auto n1 = str.length();
  auto n2 = suff.length();
  return (n1>n2 && str.substr(n1-n2) == suff);
}

// detect object class for a Detailed shape file
std::string detect_class(const std::string & base){
  int s = detect_src(base);
  if (s == 1){
    if (check_suff(base, "KarttanimiPiste")) return "text:";
    if (check_suff(base, "Piste")) return "point:";
    if (check_suff(base, "Viiva")) return "line:";
    if (check_suff(base, "Linja")) return "line:";
    if (check_suff(base, "Reuna")) return "line:";
    if (check_suff(base, "Raja")) return "line:";
    if (check_suff(base, "Alue")) return "area:";
  }
  else if (s == 2) {
    switch (base[base.size()-1]){
      case 's': return "point:";
      case 'p': return "area:";
      case 't': return "text:";
      case 'v': return "line:";
    }
  }
  throw Err() << "unknown type: " << base;
}

std::string detect_type(const Opt & opts){
  std::string ret = opts.get("Kohdeluokk",""); // [D1]
  if (ret != "") return ret;
  ret = opts.get("LUOKKA",""); // [D2]
  if (ret != "") return ret;
  std:cerr << "can't parse object type: " << opts << "\n";
  return ret;
}

/************************************************/
std::string
get_text_name(const Opt & opts){
  std::string ret = opts.get("text");
  if (ret != "") return ret;
  ret = opts.get("TEKSTI");
  return ret;
}

dPoint
get_text_pt(const Opt & opts, const dMultiLine & crd){
  if (crd.npts()<1) throw Err() << "no coordinates in a text object";
  if (opts.exists("textpositi")){ // D1
    auto pt = parse_crd(opts.get("textpositi"));
    if (dist2d(pt,crd[0][0]) > 1)
      std::cerr << "coordinates do not match: " << pt << " " << crd[0][0] << "\n";
    return pt;
  }
  // D2
  return crd[0][0] +
    dPoint(opts.get<double>("SIIRT_DX")/1000.0,
           opts.get<double>("SIIRT_DY")/1000.0);
}

dPoint
get_text_refpt(const Opt & opts, const dMultiLine & crd){
  if (crd.npts()<1) throw Err() << "no coordinates in a text object";
  if (opts.exists("placelocat")) // D1
    return parse_crd(opts.get("placelocat"));
  // D2
  return crd[0][0];
}

int
get_text_type(const Opt & opts){
  if (opts.exists("placetype")) // D1
    return opts.get<int>("placetype");

  if (opts.exists("LUOKKA")) // D2
    return opts.get<int>("LUOKKA");

  throw Err() << "Can't detect text reftype: " << opts;
}

double
get_text_angle(const Opt & opts){
  double ret = 0;
  if (opts.exists("textdirect")){ // D1
    auto p1 = parse_crd(opts.get("textpositi"));
    auto p2 = parse_crd(opts.get("textdirect"));
    if (p1 != dPoint() && p2 != dPoint())
      ret = - 180/M_PI * atan2(p2.y - p1.y, p2.x - p1.x);
  }
  if (opts.exists("SUUNTA")){ // D1
    ret = -opts.get<double>("SUUNTA")/180.0;
  }
  return ret;
}

/************************************************/


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
    std::string data_dir = args[0];
    std::string name = args[1];
    std::string out_file = args[2];
    std::string in_file = data_dir + "/" + name + ".zip";

    VMap2 vmap;
    VMap2types types(O);
    ConvGeo cnv("ETRS-TM35FIN");

    /*******************************************************/
    // Step 1: convert all text objects to vmap, with original types

    // unzip data to a temporary dir
    std::cerr << "reading data: " << in_file << "\n";
    TmpDir tmp_dir("shp2vmap_XXXXXX");
    unzip_all(tmp_dir, data_dir, name);

    for (const auto & f: file_glob({tmp_dir.get_dir() + "/*.shp"})){
      auto path = file_get_prefix(f);
      auto base = file_get_basename(f, ".shp");
      if (base.size()<1) throw Err() << "bad basename: " << f;

      int src = detect_src(base);
      std::string cl = detect_class(base);
      if (cl != "text:") continue; // only text objects

      // open shape databases
      std::cerr << "reading file: " << f << "\n";
      ShapeDB DB(path + base, 0);
      if (DB.shp_num() != DB.dbf_num())
        throw Err() << "different number of objects in SHP and DBF: " << base;

      for (size_t i = 0; i<DB.dbf_num(); ++i){
        Opt opts = DB.get_opts(i);

        // skip second language (D1)
        if (opts.get<int>("languagedo") > 1) continue;

        // Find type info:
        ref_type_t tinfo = {"point:34600", 1, 0.75}; // default
        int reftype = get_text_type(opts);
        if (ref_types.count(reftype)!=0)
          tinfo = ref_types[reftype];
        else {
          std::cerr << "Unknown text type: " << reftype << "\n";
          continue;
        }

        if (tinfo.type=="-") continue;

        // Make a temporary vmap file:
        // all objects with original reftypes, original coords
        // set elevation in the point
        VMap2obj obj1("text:1");
        obj1.name = get_text_name(opts);
        if (obj1.name == "") continue;
        obj1.add_point(get_text_pt(opts, DB.get(i)));
        obj1[0][0].z = opts.get<double>("placeeleva", NAN);
        obj1.ref_pt = get_text_refpt(opts, DB.get(i));
        obj1.set_ref_type(tinfo.type);
        obj1.angle = get_text_angle(opts);
        obj1.scale = opts.get<int>("textsize", 200)/200.0; // 1 for D2

        if (!std::isnan(tinfo.scale)) obj1.scale = tinfo.scale;
        if (!tinfo.angle) obj1.angle = NAN;

        if (tinfo.legend){
          obj1.name[0] = tolower(obj1.name[0]); // TODO - utf8
          obj1.name = "\\" + obj1.name;
        }
        if (ref_types.count(reftype)==0)
          obj1.name = obj1.name + ":" + type_to_str(reftype);

        if (src==1) obj1.tags.insert("D1");
        else obj1.tags.insert("D2");

        // find duplicates
        /*******************************************************/
        // Step 2: duplicates
        // - one refpoint can have multiple labels with same name (long river).
        //   We want to keep them.
        // - D1 object can have name splitted into multiple lines.
        //   We want to merge them in correct order (highest to lowest).
        // - D2 object can have many language labels.
        //   We want to keep the first (highest) one.
        // - D2 object can have both label and legend.
        //   We want to merge them.
        double ref_acc = 10; // m
        vmap.iter_start();
        while (!vmap.iter_end()){
          auto p2 = vmap.iter_get_next();
          auto id2  = p2.first;
          auto obj2 = p2.second;
          if (obj1.ref_type != obj2.ref_type ||
              (int)obj1.ref_type - obj2.ref_type) continue;
          if (dist2d(obj1.ref_pt, obj2.ref_pt) > ref_acc) continue;

          // We found point with same reference.
          // If names are different, merge them (upper label goes first).
          auto & n1 = obj1.name;
          auto & n2 = obj2.name;
          if (n1!=n2 && n1.size() && n2.size()){
            auto & p1 = obj1[0][0];
            auto & p2 = obj2[0][0];
            // upper label -> name1
            if (p2.y>p1.y) n1.swap(n2);

            if (src==1){
              if (n1[n1.size()-1]=='-') n1.resize(n1.size()-1);
              else n1+=' ';
              n2 = n1 + n2;
            }
            else {
              n2 = n1 + '\t' + n2;
            }
            n1 = ""; // we want to skip this text
            vmap.put(id2, obj2);
          }
          else {obj1.tags.insert("dup");}
        }
        if (obj1.name!="") vmap.add(obj1);
      }
    }

    /*******************************************************/
    // Step 2: some tuning

    vmap.iter_start();

    for (const auto id: vmap.find("text:1")){
      auto obj = vmap.get(id);

      // shorten name
      auto n = obj.name.find(" eli ");
      if (n != std::string::npos)
        obj.name = obj.name.substr(0,n);

      // attach altitude to summit names
      if (!std::isnan(obj[0][0].z) &&
          obj.ref_type == obj.make_type("point:0x1100"))
        obj.name = type_to_str<int>(obj[0][0].z) + " " + obj.name;

      // single legend
      if (obj.name.find('\\')==0)
        obj.name = obj.name.substr(1);

      // process multi-line text in D2
      if (obj.name.find('\t')!=obj.name.npos){
        // keep first part
        auto n1 = obj.name.substr(0,obj.name.find('\t'));
        // last part
        auto n2 = obj.name.substr(obj.name.rfind('\t')+1);
        if (n2.size() && n2[0]=='\\')
          obj.name = n1+n2;
        else
          obj.name = n1;
      }

      // transfer tags
      auto tags = obj.tags;
      obj.tags.clear();

      // convert coords and save object
      cnv.frw(obj);
      cnv.frw(obj.ref_pt);
      vmap.put(id, obj);

      if (tags.count("dup")) {
        types[obj.ref_type].label_maxnum = -1;
        continue;
      }

      // create ref object
      VMap2obj objr;
      objr.add_point(obj.ref_pt);
      objr.type = obj.ref_type;
      objr.name = obj.name;
      objr.tags = tags;
      vmap.add(objr);


    }

    /********************************************************/

    do_update_labels(vmap, types);
    vmap2_export(vmap, types, out_file, Opt());
  }

  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}

