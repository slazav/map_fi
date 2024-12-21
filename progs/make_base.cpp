#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "filename/filename.h"
#include "shape/shp.h"
#include "shape/dbf.h"
#include "geo_data/conv_geo.h"
#include "tmpdir/tmpdir.h"
#include "geom/poly_tools.h"

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"

using namespace std;

// Read original zip archive <name>.zip with shape files,
// create base vmap (without text objects).

// Object groups with type.
// Used for constructing filenames for reading data.
// Type was important if one does not want to use type
// substitution table and read all types directly.
std::map<std::string, std::string> map_groups = {
  {"AmpumaRaja",           "line"},
  {"HallintoalueRaja",     "line"},
  {"HallintoAlue",         "area"},
  {"KarttanimiPiste",      "text"},
  {"KiitotieViiva",        "line"},
  {"KorkeusViiva",         "line"}, // Contours (lines)
  {"KoskiPiste",          "point"},
  {"KoskiViiva",           "line"},
  {"LiikenneAlue",         "area"},
  {"MaaAlue",              "area"},
  {"Maasto1Reuna",         "line"},
  {"Maasto1Viiva",         "line"},
  {"Maasto2Alue",          "area"},
  {"MajakkaPiste",        "point"},
  {"MetsaRaja",            "line"},
  {"PeltoAlue",            "area"},
  {"PorttiPiste",         "point"},
  {"RajavyohykeRaja",      "line"},
  {"RakennusAlue",         "area"},
  {"RakennusPiste",       "point"},
  {"RakennusViiva",        "line"}, // Some constructions (lines): fence, ski lift, etc.
  {"RautatiePiste",       "point"}, // Railroads (lines)
  {"RautatieViiva",        "line"},
  {"SahkoLinja",           "line"},
  {"SahkoPiste",          "point"},
  {"SuojaAlue",            "area"},
  {"SuojametsaRaja",       "line"},
  {"SuojeluAlue",          "area"},// Borders of natural areas (polygons)
  {"TaajamaAlue",          "area"},
  {"TiePiste",            "point"},// Road symbols?
  {"TieViiva",             "line"}, // Roads (lines)
  {"VesiAlue",             "area"},
  {"VesiliikenneViiva",    "line"},
  {"VesirakennelmaPiste", "point"},
  {"VesirakennelmaViiva",  "line"},
  {"VesiViiva",            "line"},
};

// Type substutution list. Used for:
// - Substituting type by something suteable in mp/img.
// - Check for unknown types (can be important).
// Final map can contain other types, this is used only
// for importing initial data.
// If destination is empty, type is silently skipped.
// If destination is '+' type is not substituded.
std::map<std::string, std::string> type_subst = {
 {"area:72200", "line:0x37"}, // нац.парк
 {"area:72201", "line:0x37"}, // природный парк
 {"area:72202", "line:0x37"}, // заповедник
 {"area:72700", "+"}, // дикая территория
 {"area:84200", "+"}, // муниципалитет

 {"line:84111", "line:0x1D"}, // гос.граница
 {"line:82200", "line:0x36"}, // граница погранзоны
 {"line:84113", "+"}, // муниципальная граница
 {"line:72500", "+"}, // охраняемый лес - граница
 {"line:39500", "+"}, // граница зоны леса (глобальная)

 {"area:39120", "area:0x14"}, // кустарник
 {"area:32611", "area:0x52"}, // поле
 {"area:32800", "area:0x52"}, // луг

 {"area:35411", "area:0x54"}, // открытая заболоченность
 {"area:35412", "area:0x51"}, // лесная заболоченность
 {"area:35421", "area:0x55"}, // открытое болото
 {"area:35422", "area:0x4C"}, // лесное болото

 {"area:34100", "area:0x19"}, // скалы
 {"area:34300", "area:0xD"},  // песок
 {"area:34700", "area:0x8"},  // галечник, каменные россыпи

 {"area:32900", "area:0x56"}, // парк
 {"area:33100", "area:0x57"}, // спортплощадка

 {"area:32500", "area:0x58"}, // карьер
 {"area:33000", "area:0x58"}, // отвалы грунта
 {"area:32111", "area:0x58"}, // площадь для добычи песка, щебня

 {"line:52100", "line:0x21"}, // горизонталь

 {"area:32421", "area:0x59"}, // парковка, проезжая зона
 {"area:32441", "area:0x04"}, // зона аэропорта
 {"area:38900", "area:0x04"}, // склад
 {"area:42243", "area:0x04"}, // стрельбище?

 {"area:32200", "area:0x1A"}, // кладбище

 {"area:40200", "area:0x0E"}, // населенка

 {"area:36200", "area:0x29"}, // озеро
 {"area:36211", "area:0x29"}, // море
 {"area:36315", "area:0x29"}, // площадная река
 {"area:36316", "area:0x29"}, // площадной канал
 {"area:44300", "area:0x29"}, // бассейн

 {"line:36311", "line:0x15"}, // ручей менее 2м
 {"line:36312", "line:0x18"}, // ручей 2-5м
 {"line:36314", "line:0x1F"}, // ручей >5м
 {"line:16511", "line:0x41"}, // корабельный канал
 {"line:16512", "line:0x40"}, // лодочный канал
 {"line:30100", "line:0x42"}, // искусственная береговая линия
 {"line:30222", "line:0x43"}, // нечеткая береговая линия
 {"line:38200", "line:0x44"}, // порог линейный

 {"line:12121", "line:0x01"}, // шоссе 2a
 {"line:12122", "line:0x0B"}, // шоссе 2b
 {"line:12131", "line:0x02"}, // шоссе 3a
 {"line:12132", "line:0x04"}, // шоссе 3b
 {"line:12141", "line:0x06"}, // дорога

 {"line:22311", "line:0x29"}, // ЛЭП > 110kV
 {"line:32431", "+"}, // ВВП аэропорта

 {"line:30300", "line:0x2C"}, // плотина
 {"line:34400", "line:0x03"}, // утес
 {"line:44211", "line:0x19"}, // забор
 {"line:44500", "+"}, // подъемник

 {"point:38200", "point:0x650E"}, // порог
 {"point:12198", "+"}, // круг
 {"point:22200", "point:0x6402"}, // распределительная станция
 {"point:42213", "point:0x6402"}, // дом
 {"point:42223", "point:0x6402"}, // дом
 {"point:42233", "point:0x6402"}, // дом
 {"point:42243", "point:0x6402"}, // дом
 {"point:42253", "point:0x6402"}, // дом
 {"point:42270", "point:0x2C0B"}, // церковь
 {"point:44800", "point:0x6411"}, // мачта
 {"point:45300", "point:0x6411"}, // высокая труба
 {"point:45200", "point:0x6401"}, // ворота, проход в заборе
};


GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "shp2vmap");
  pr.name("Read shape files from maanmittauslaitos.fi");
  pr.usage("[<options>] <in_file> <out_file>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD", "VMAP2"});
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

    VMap2 vmap, vmap_t;
    ConvGeo cnv("ETRS-TM35FIN");

    // unzip data to a temporary dir
    std::cerr << "reading data: " << in_file << "\n";
    TmpDir tmp_dir("shp2vmap_XXXXXX");
    tmp_dir.unzip(in_file);
    std::string name = file_get_basename(in_file);


    // go through all files
    for (const auto & map_group: map_groups){

      // skip text objects (see make_text prog)
      if (map_group.second == "text") continue;

      // open shape databases
      std::string base = tmp_dir.get_dir() + "/" + name + "_" + map_group.first;
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


      // go through all objects
      for (size_t i = 0; i<nr; ++i){

        // collect object info to obj
        Opt o;
        for (size_t j = 0; j<nf; ++j)
          o.put<std::string>(DB.field_name(j), DB.get_str(i,j));

        // map type
        std::string type = o.get("Kohdeluokk","");

        if (type == "") {
          std:cerr << "can't parse object type: "
                   << base << ": " << i << "\n";
          continue;
        }

        // add line:, area:, point: to type
        type=map_group.second + ":" + type;

        // check type
        if (type_subst.count(type)==0)
          throw Err() << "Please add unknown type: " << type;

        // skip or substitute type
        if (type_subst[type]=="" || type_subst[type]=="-") continue;
        if (type_subst[type]!="+") type = type_subst[type];

        // create object and set coordinates
        VMap2obj obj(type);
        obj.dMultiLine::operator=(SH.get(i));

        // contour names
        if (o.exists("Korkeus")){ 
          double k = o.get<double>("Korkeus");
          if (k>-1000 && k<10000)
            obj.name = o.get("Korkeus");
          if (int(k)%50 == 0)
            obj.set_type("line:0x22");
        }

        // filter points

        double acc = 10.0; //m
        if (acc>0) {
          if (obj.npts()>2 && obj.length()>2*acc)
            line_filter_rdp(obj, acc);
          if (obj.npts()==0) continue;
        }

        cnv.frw(obj);
        vmap.add(obj);
      }
    }

    // read file with type information if it's available
    VMap2types types(O);
    vmap2_export(vmap, types, out_file, Opt());
  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
