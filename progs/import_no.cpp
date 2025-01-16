#include <map>
#include <string>
#include <fstream>
#include "geom/multiline.h"
#include "geom/line_rectcrop.h"
#include "getopt/getopt.h"
#include "getopt/help_printer.h"
#include "geo_data/conv_geo.h"
#include "geo_data/geo_io.h"
#include "geo_nom/geo_nom_fi.h"
#include "vmap2/vmap2io.h"
#include "gis/ewkb.h"
#include "filename/filename.h"
#include "geom/poly_tools.h"
#include <regex>

#include "lib.h"

using namespace std;

// read original Norwegian N50 PostGis sql data


std::map<std::string, std::string> tr = {
 {"allmenning", "public"},
 {"allmenninggrense", "public_boundary"},
 {"allmenningtype", "public_type"},
 {"alpinbakke", "alpine_hill"},
 {"anleggstype", "facility_type"},
 {"apentomrade", "open_area"},
 {"arealbrukgrense", "landuse_boundary"},
 {"avtaltavgrensningslinje", "agreement_boundary"},
 {"bane", "track"},
 {"banestatus", "track_status"},
 {"betjeningsgrad", "service_level"},
 {"bygning_omrade", "building_area"},
 {"bygning_posisjon", "building_position"},
 {"bygningstype", "building_type"},
 {"bygningstypekode", "building_typecode"},
 {"bymessigbebyggelse", "urban_development"},
 {"campingplass", "campsite"},
 {"dataavgrensning", "data_boundary"},
 {"datafangstdato", "data_date"},
 {"dyrketmark", "cultivated_land"},
 {"elv", "river"},
 {"elvbekk", "stream"},
 {"elvekant", "river_bank"},
 {"elvelinjefiktiv", "river_linefictitious"},
 {"elvmidtlinje", "river_cline"},
 {"ferskvanntorrfall", "freshwater_dryfall"},
 {"ferskvanntorrfallkant", "freshwater_dryfall_edge"},
 {"fiktivdelelinje", "fictitious_dividing_line"},
 {"fler_linjer", "multiple_lines"},
 {"flipangle", "flipangle"},
 {"flomlopkant", "floodplain"},
 {"flytebrygge", "floating_pier"},
 {"forsenkningskurve", "subsidence_curve"},
 {"foss", "waterfall"},
 {"fulltekst", "fulltext"},
 {"fylkesgrense", "county_boundary"},
 {"fylkesnavn", "county_name"},
 {"fylkesnummer", "county_number"},
 {"geometri", "geometry"},
 {"golfbane", "golf_course"},
 {"gravplass", "cemetery"},
 {"grense", "border"},
 {"grensepunktnummer", "border_point_number"},
 {"grensepunkttype", "border_point_type"},
 {"grunnlinje", "baseline"},
 {"grunnlinjepunkt", "baseline_point"},
 {"grunnlinjepunktnavn", "baseline_point_name"},
 {"grunnlinjepunktnummer", "baseline_point_number"},
 {"gruve", "mine"},
 {"havelvsperre", "sea_barrier"},
 {"havflate", "sea_surface"},
 {"havinnsjosperre", "sea_lake_barrier"},
 {"hjelpekurve", "auxiliary_curve"},
 {"hoppbakke", "jump_hill"},
 {"hoyde", "height"},
 {"hoydekurve", "height_curve"},
 {"hytteeier", "cabin_owner"},
 {"iatakode", "iata_code"},
 {"icaokode", "icao_code"},
 {"identifier", "identifier"},
 {"industriomrade", "industrial_area"},
 {"innsjo", "lake"},
 {"innsjoelvsperre", "lake_river_barrier"},
 {"innsjoinnsjosperre", "lake_lake_barrier"},
 {"innsjokant", "lake_edge"},
 {"innsjokantregulert", "lake_edge_regulated"},
 {"innsjomidtlinje", "lake_center_line"},
 {"innsjoregulert", "lake_regulated"},
 {"jernbanetype", "railway_type"},
 {"kaibrygge", "quay_pier"},
 {"kanalgroft", "channel_rough"},
 {"kantutsnitt", "edge_section"},
 {"kommune", "municipality"},
 {"kommunegrense", "municipality_boundary"},
 {"kommunenavn", "municipality_name"},
 {"kommunenummer", "municipality_number"},
 {"kystkontur", "coastal_contour"},
 {"lavesteregulertevannstand", "lowest_regulated_water_level"},
 {"ledning", "line"},
 {"lengde", "length"},
 {"lufthavneier", "airport_owner"},
 {"lufthavn_omrade", "airport_area"},
 {"lufthavn_posisjon", "airport_position"},
 {"lufthavntype", "airport_type"},
 {"luftledninglh", "airline_line"},
 {"lysloype", "lighting_track"},
 {"malemetode", "painting_method"},
 {"masttele", "tele_mast"},
 {"mediumhoyde", "medium_height"},
 {"mediumsamferdsel", "medium_traffic"},
 {"molo", "breakwater"},
 {"motorvegtype", "motorway_type"},
 {"myr", "marsh"},
 {"naturverngrense", "nature_conservation_boundary"},
 {"naturvernomrade", "nature_conservation_area"},
 {"naturvernpunkt", "nature_conservation_point"},
 {"navigasjonsinstallasjon", "navigation_installation"},
 {"navn", "name"},
 {"navneobjektgruppe", "nameobject_group"},
 {"navneobjekthovedgruppe", "nameobject_main_group"},
 {"navneobjekttype", "name_object_type"},
 {"noyaktighet", "accuracy"},
 {"omrade", "area"},
 {"oppdateringsdato", "update_date"},
 {"parkeringsomrade", "parking_area"},
 {"pir", "pier"},
 {"posisjon", "position"},
 {"presentasjontekst", "presentation_text"},
 {"referansemalestokk", "reference_template"},
 {"reingjerde", "reindeer_fence"},
 {"retningsenhet", "direction_unit"},
 {"retningsreferanse", "direction_reference"},
 {"retningsverdi", "direction_value"},
 {"riksgrense", "national_border"},
 {"rorgate", "rudder_gate"},
 {"rullebane", "runway"},
 {"rutemerking", "route_marking"},
 {"senterlinje", "center_line"},
 {"skitrekk", "ski_lift"},
 {"skjer", "skerk"},
 {"skog", "forest"},
 {"skriftkode", "font_code"},
 {"skrifttype", "font_type"},
 {"skrivematenummer", "font_type_number"},
 {"skytebaneinnretning", "shooting_range_facility"},
 {"skytefelt", "shooting_range"},
 {"skytefeltgrense", "shooting_range_boundary"},
 {"skytefeltstatus", "shooting_range_status"},
 {"snoisbre", "snow_glacier"},
 {"spesielldetalj_posisjon", "special_detail_position"},
 {"spesielldetalj_senterlinje", "special_detail_line"},
 {"sporantall", "track_number"},
 {"sportidrettplass", "sports_ground"},
 {"sprak", "language"},
 {"sprakkode", "language_code"},
 {"sprakprioritering", "language_priority"},
 {"sprakprioriteringkode", "language_priority_code"},
 {"stasjon", "station"},
 {"stedsnavnnummer", "place_name_number"},
 {"stedsnavntekst", "place_name_text"},
 {"stedsnummer", "place_number"},
 {"steinbrudd", "quarry"},
 {"steintipp", "stone_tip"},
 {"streng", "string"},
 {"takkant", "roof_edge"},
 {"tankkant", "tank_edge"},
 {"tank_omrade", "tank_area"},
 {"tank_posisjon", "tank_position"},
 {"tarn", "tower"},
 {"taubane", "cableway"},
 {"tegnavstand", "character_distance"},
 {"teiggrensepunkt", "territorial_boundary_point"},
 {"tekstreferansepunktnord", "text_reference_point_north"},
 {"tekstreferansepunktost", "text_reference_point_east"},
 {"teksttype", "text_type"},
 {"terrengpunkt", "terrain_point"},
 {"territorialgrense", "territorial_boundary"},
 {"tettbebyggelse", "urban_settlement"},
 {"tilgjengelighet", "accessibility"},
 {"trafikktype", "traffic_type"},
 {"tregruppe", "tree_group"},
 {"trigonometriskpunkt", "trigonometric_point"},
 {"typeveg", "type_road"},
 {"vannbredde", "water_width"},
 {"vatnlopenummer", "watercourse_number"},
 {"vedlikeholds", "maintenance"},
 {"vedlikeholdsansvarlig", "maintenance_manager"},
 {"vegfase", "road_phase"},
 {"vegkategori", "road_category"},
 {"veglenke", "road_link"},
 {"vegnummer", "road_number"},
 {"vegsperring", "road_closure"},
 {"vegsperringtype", "road_closure_type"},
 {"vernedato", "protection_date"},
 {"verneform", "protection_form"},
 {"verticalalignment", "vertical_alignment"},
 {"vindkraftverk", "wind_turbine"},
};

std::map<std::string, std::set<std::string> > maps = {
{"V34", {"Troms"}},
{"V42", {"Finnmark"}},
{"V44", {"Finnmark"}},
{"W33", {"Troms", "Finnmark"}},
{"W34", {"Troms"}},
{"W41", {"Troms", "Finnmark"}},
{"W42", {"Troms", "Finnmark"}},
{"W43", {"Finnmark"}},
{"W44", {"Finnmark"}},
{"W51", {"Finnmark"}},
{"W52", {"Finnmark"}},
{"X41", {"Troms", "Finnmark"}},
{"X42", {"Troms", "Finnmark"}},
{"X43", {"Finnmark"}},
{"X44", {"Finnmark"}},
{"X51", {"Finnmark"}},
{"X52", {"Finnmark"}},
};

std::map<std::string, std::string> regions = {
  {"Troms",    "Basisdata_55_Troms_25833_N50Kartdata_PostGIS.sql"},
  {"Finnmark", "Basisdata_56_Finnmark_25835_N50Kartdata_PostGIS.sql"},
};

std::map<std::string, std::string> cnvs = {
  {"Troms",    "EPSG:25833"},
  {"Finnmark", "EPSG:25835"},
};



std::vector<std::string>
split(const std::string & str, const std::string & del) {
  std::vector<std::string> ret;
  size_t last = 0;
  size_t next = 0;
  while ((next = str.find(del, last)) != str.npos) {
    ret.push_back(str.substr(last, next-last));
    last = next + del.length();
  }
  ret.push_back(str.substr(last));
  return ret;
}

class SqlReader {
  bool input=false;
  std::vector<std::string> fields;
  std::string table;
  std::ifstream ff;

  public:
    SqlReader(const std::string & fname): input(false), ff(fname) {}

    Opt get_data(){
      while (ff){
        std::string line;
        std::getline(ff, line);

        // COPY <schema>.<table> (<field1>, <field2>, ...) FROM stdin;
        if (line.substr(0,4) == "COPY"){
          auto n1 = line.find('.');
          auto n2 = line.find(' ',n1+1);
          table = line.substr(n1+1,n2-n1-1);
          if (tr.count(table)) table = tr[table];

          n1 = line.find('(');
          n2 = line.find(')',n1+1);
          fields = split(line.substr(n1+1,n2-n1-1), ", ");
          for (auto & f: fields) if (tr.count(f)) f = tr[f];
          input = true;
          continue;
        }
        if (line.substr(0,2) == "\\.") {
          input = false;
          fields.clear();
          table = "";
          continue;
        }
        if (input){
          auto vals = split(line, "\t");
          if (vals.size()!=fields.size())
            throw Err() << "wrong number of fields";
          Opt ret;
          ret.put("table", table);
          for (size_t i = 0; i<vals.size(); ++i) ret.put(fields[i], vals[i]);
          return ret;
        }
      }
      return Opt();
    }
    operator bool() const{ return (bool)ff; }
};

/***********************************************************/

GetOptSet options;

void usage(bool pod=false){
  HelpPrinter pr(pod, options, "import_no");
  pr.name("Read PostGis sql data from https://kartkatalog.geonorge.no");
  pr.usage("[<options>] <name>");
  pr.head(2, "Options:");
  pr.opts({"HELP","POD"});
  throw Err();
}

int
main(int argc, char *argv[]){
  try{
    ms2opt_add_std(options, {"HELP","POD"});

    vector<string> args;
    Opt O = parse_options_all(&argc, &argv, options, {}, args);
    if (O.exists("help")) usage();
    if (O.exists("pod"))  usage(true);

    if (args.size() != 1) usage();
    std::string name  = args[0];

    // expand cropping region be 100m to avoid side effects on the border
    dRect box = nom_to_range_fi(name);
    box.expand(100);

    auto ofile = name + "_no1.vmap2.gz";

    VMap2 vmap;

    for (const auto & reg: maps[name]){
      if (regions.count(reg)<1)
        throw Err() << "undefined region: " << reg;

      auto ifile = regions[reg];
      if (!file_exists(ifile))
        throw Err() << "file does not exist: " << ifile;

      ConvGeo cnv1(cnvs[reg], "ETRS-TM35FIN");
      ConvGeo cnv2("ETRS-TM35FIN");

      std::string t0 = "";
      int  nn=0;
      dMultiLine brd;
      SqlReader reader(ifile);
      std::cerr << "reading: " << reg << "\n";
      while (reader) {
        auto o = reader.get_data();

        std::string table = o.get("table");

        // detect coordinate field
        std::string cfield = "";
        std::string cl = "";
        std::string ty = "1"; // default type
        if (o.exists("area"))             {cl="area";  cfield = "area";}
        else if (o.exists("border"))      {cl="line";  cfield = "border";}
        else if (o.exists("center_line")) {cl="line";  cfield = "center_line";}
        else if (o.exists("geometry"))    {cl="line";  cfield = "geometry";}
        else if (o.exists("position"))    {cl="point"; cfield = "position";}
        else continue; // skip records w/o coords

        // convert coordinates, crop to range, skip objects outside the range
        auto crds = ewkb_decode(o.get(cfield), false, false);
        cnv1.frw(crds); // -> FI

        if (table=="data_boundary"){
          brd.insert(brd.end(), crds.begin(), crds.end());
          continue;
        }

        // crop to FI map (area objects are closed lines!)
        crds = rect_crop_multi(box, crds, cl=="area");
        if (crds.size()==0) continue;


        // print all new tables
        if (table != t0){
          if (nn>0) std::cerr << t0 << ": " << nn << "objects\n";
          t0 = table; nn=0;
        }
        nn++;

        // add type1/type2 fields for text
        std::vector<std::string> cnv_flds =
          {"name_object_type", "nameobject_group", "nameobject_main_group", "text_type",
           "type_road"};
        for (auto & fld: cnv_flds){
          if (!o.exists(fld)) continue;
          auto s = o.get(fld);
          s = std::regex_replace(s, std::regex(u8"ø"), "o"); // &oslash;
          s = std::regex_replace(s, std::regex(u8"æ"), "ae"); // &aelig;
          s = std::regex_replace(s, std::regex(u8"å"), "a"); // &aring;
          o.put(fld, s);
        }

        // text type for
        if (o.exists("name_object_type")){
          std::string t1 = o.get("name_object_type");
          std::string t2 = o.get("nameobject_main_group") + "/"
                         + o.get("nameobject_group") + "/"
                         + o.get("name_object_type");
          o.put("text_type", t1);
          o.put("text_fulltype", t2);
          o.erase("name_object_type");
          o.erase("nameobject_main_group");
          o.erase("nameobject_group");
        }

        // remove some defaults:

        for (auto i=o.begin(); i!=o.end();){
          if (i->second == "" ||
              i->second == "\\N" ||
              i->first.find("fmx_")==0 || // some label variant
              i->first.find("geodb_")==0 // some label variant
          ) i=o.erase(i);
          else ++i;
        }
        opt_del_def(o, "data_date",   "1000-01-01");
        opt_del_def(o, "update_date", "1000-01-01");
        opt_del_def(o, "angle", "0");
        opt_del_def(o, "bold", "0");
        opt_del_def(o, "override", "0");
        opt_del_def(o, "reference_template", "50000");
        opt_del_def(o, "xoffset", "0");
        opt_del_def(o, "yoffset", "0");


        o.erase(cfield);
        o.put("crd_field", cfield);

        VMap2obj obj(cl + ":" + ty);
        obj.opts = o;

        // text objects (2 coordinates!)
        if (table == "place_name_text") obj.set_type("text:1");
        if (table == "presentation_text") obj.set_type("text:2");

        // filter points
        if (obj.get_class() == VMAP2_LINE ||
            obj.get_class() == VMAP2_POLYGON){
          line_filter_rdp(crds, 10); // m
          if (crds.npts()==0) continue;
        }
        obj.set_coords(cnv2.frw_acc(crds)); // -> WGS84, accurate

        obj.dMultiLine::operator=(crds);
        vmap.add(obj);
      }
      if (nn>0) std::cerr << t0 << ": " << nn << "objects\n";

      {
        GeoData D;
        D.push_back(GeoTrk(cnv2.frw_acc(brd)));
        write_gpx(reg + ".gpx", D);
      }
    }

    if (vmap.size()) vmap2_export(vmap, VMap2types(), ofile, Opt());
  }
  catch(Err & e){
    if (e.str()!="") cerr << "Error: " << e.str() << "\n";
    return 1;
  }
}
