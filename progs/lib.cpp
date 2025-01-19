#include "lib.h"

#include "err/err.h"
#include "filename/filename.h"

// make new reference object for a text object:
VMap2obj
make_ref_obj(const VMap2obj & objt, const std::string & src){
  VMap2obj obj(objt.ref_type);
  obj.add_point(objt.ref_pt);
  obj.name = objt.name;
  if (src!="") obj.opts.put("Source", src);
  return obj;
}

/********************************************************************/

void
read_src(VMap2 & vmap, const std::string & fname){
  if (!file_exists(fname)){
    std::cerr << "skip src: " << fname << "\n";
    return;
  }
  std::cerr << "read src: " << fname << "\n";
  vmap2_import(fname, VMap2types(), vmap, Opt());
}

void
read_src_fi2(VMap2 & vmap, const std::string & data_dir,
             const std::string & name){
  const char LR[] = "LR";
  const char I234[] = "1234";
  for (int n1 = 0; n1<4; ++n1){
    for (int n2 = 0; n2<4; ++n2){
      for (int n3 = 0; n3<2; ++n3){
        std::string ifile = data_dir + "/" + name + I234[n1] + I234[n2] + LR[n3] +".vmap2.gz";
        if (!file_exists(ifile)) {
          std::cerr << "skip src: " << ifile << "\n";
          continue;
        }
        std::cerr << "read src: " << ifile << "\n";
        vmap2_import(ifile, VMap2types(), vmap, Opt());
      }
    }
  }
}

/********************************************************************/

std::map<std::string, std::string>
read_tconv_str(const std::string & fname){
  std::map<std::string, std::string> ret;
  std::ifstream ff(fname);
  if (!ff) throw Err() << "can't open file: " << fname;
  int line_num[2] = {0,0}; // line counter for read_words
  while (1){
    auto vs = read_words(ff, line_num, false);
    if (vs.size()==0) break;
    try{
      if (vs.size()!=2) throw Err()
        << "two words expected: <src_type> <dst_type>";
      ret[vs[0]] = vs[1];
    }
    catch (Err & e) {
      throw Err() << fname << ":" << line_num[0] << ": " << e.str();
    }
  }
  return ret;
}

std::map<std::string, tconv_fi2_t>
read_tconv_fi2(const std::string & fname){
  std::map<std::string, tconv_fi2_t> ret;
  std::ifstream ff(fname);
  if (!ff) throw Err() << "can't open file: " << fname;
  int line_num[2] = {0,0}; // line counter for read_words
  while (1){
    auto vs = read_words(ff, line_num, false);
    if (vs.size()==0) break;
    try{
      if (vs.size()!=3) throw Err()
        << "two words expected: <src_type> <action> <dst_type>";
      ret[vs[0]] = {vs[1], vs[2]};
    }
    catch (Err & e) {
      throw Err() << fname << ":" << line_num[0] << ": " << e.str();
    }
  }
  return ret;
}

std::map<std::string, tconv_txt_t>
read_tconv_txt(const std::string & fname){
  std::map<std::string, tconv_txt_t> ret;
  std::ifstream ff(fname);
  if (!ff) throw Err() << "can't open file: " << fname;
  int line_num[2] = {0,0}; // line counter for read_words
  while (1){
    auto vs = read_words(ff, line_num, false);
    if (vs.size()==0) break;
    try{
      if (vs.size()!=5) throw Err()
        << "5 words expected: <src_type> <dst_type> <ref_type> <keep_angle> <scale>";
      ret[vs[0]] = {vs[1], vs[2], str_to_type<int>(vs[3]), vs[4]};
    }
    catch (Err & e) {
      throw Err() << fname << ":" << line_num[0] << ": " << e.str();
    }
  }
  return ret;
}

std::map<std::string, tconv_fi2t_t>
read_tconv_fi2t(const std::string & fname){
std::map<std::string, tconv_fi2t_t> ret;
  std::ifstream ff(fname);
  if (!ff) throw Err() << "can't open file: " << fname;
  int line_num[2] = {0,0}; // line counter for read_words
  while (1){
    auto vs = read_words(ff, line_num, false);
    if (vs.size()==0) break;
    try{
      if (vs.size()!=6) throw Err()
        << "6 words expected: <src_type> <dst_type> <ref_type> <legend> <keep_angle> <scale>";
      ret[vs[0]] =
        {vs[1], vs[2], str_to_type<int>(vs[3]), str_to_type<int>(vs[4]), vs[5]};
    }
    catch (Err & e) {
      throw Err() << fname << ":" << line_num[0] << ": " << e.str();
    }
  }
  return ret;
}

std::list<oconv_t>
read_oconv(const std::string & fname){
std::list<oconv_t> ret;
  std::ifstream ff(fname);
  if (!ff) throw Err() << "can't open file: " << fname;
  int line_num[2] = {0,0}; // line counter for read_words
  while (1){
    auto vs = read_words(ff, line_num, false);
    if (vs.size()==0) break;
    try{
      if (vs.size()<6) throw Err()
        << "at least 6 words expected: (<map>|*) (<type>|*) (<lat>|*) (<lon>|*) <name> <action> [parameters]";
      ret.emplace_back(vs);
    }
    catch (Err & e) {
      throw Err() << fname << ":" << line_num[0] << ": " << e.str();
    }
  }
  return ret;
}


/********************************************************************/

// extend objects beyond map bounaries
void
extend_object(dMultiLine & crds, const dRect & box){
  for (auto & l:crds){
    for (auto i1=l.begin(); i1!=l.end(); ++i1){
      auto i2 = (i1+1 == l.end())? l.begin() : i1+1;
      if (box.contains_n(*i1) && !box.contains_n(*i2))
        *i2 = *i2 + norm(*i2-*i1)*100;
      if (box.contains_n(*i2) && !box.contains_n(*i1))
        *i1 = *i1 + norm(*i1-*i2)*100;
      // should we proccess points with both neighbours on the border?
    }
  }
}

bool
check_suff(const std::string & str, const std::string & suff){
  auto n1 = str.length();
  auto n2 = suff.length();
  return (n1>n2 && str.substr(n1-n2) == suff);
}

void
opt_mv_dbl(Opt & opts, const std::string & src, const std::string & dst){
  if (!opts.exists(src)) return;
  auto v = opts.get<double>(src);
  opts.erase(src);
  opts.put(dst, v);
}

void
opt_mv_txt(Opt & opts, const std::string & src, const std::string & dst){
  if (!opts.exists(src)) return;
  int v = opts.get<int>(src);
  opts.erase(src);
  opts.put(dst, v);
}

void
opt_del_def(Opt & opts, const std::string & src, const std::string & def){
  if (!opts.exists(src)) return;
  if (opts.get(src) == def) opts.erase(src);
}

/********************************************************************/

#include "geo_data/geo_io.h"
#include "geom/poly_tools.h"

void
crop_to_border(VMap2 & vmap, const std::string & fname){

  // read border polygon from file
  GeoData d;
  read_geo(fname, d);
  if (d.trks.size()<1) {
    std::cerr << "can't find border file: " << fname;
    return;
  }
  dLine brd = *d.trks.begin();
  brd.flatten();

  dPolyTester tst(brd);
  dPoint v;

  vmap.iter_start();
  while (!vmap.iter_end()){
    auto pp = vmap.iter_get_next();
    auto i = pp.first;
    auto obj = pp.second;

    // point objects
    if (obj.is_class(VMAP2_POINT)){
      auto pt = obj.get_first_pt();

      // remove border poles
      if (obj.is_type("point:82500")){
        if (nearest_pt(brd, v, pt, 1/1200.0) < 1/1200.0) vmap.del(i);
        continue;
      }
      if (tst.test_pt(pt)) vmap.del(i);
      continue;
    }

    // crop only line objects
    if (!obj.is_class(VMAP2_LINE)) continue;
    bool is_brd = obj.is_type("line:0x1D");

bool z = obj.is_type("line:0x19");


    auto mod = false;
    auto seg = obj.begin();
    while (seg!=obj.end()){
      dMultiLine ml;
      auto i1 = seg->begin(), i2 = seg->begin();
      while (i2!=seg->end()){

        dPoint pt = *i2;
        bool keep = true;
        // keep border points: far from our border
        if (is_brd && nearest_pt(brd, v, pt, 1/1200.0) < 1/1200.0) keep=false;
        // keep non border points: outside our border or near out border
        if (!is_brd && tst.test_pt(*i2) && nearest_pt(brd, v, pt, 1/1200.0) > 1/1200.0) keep=false;
        if (!keep){
          if (i1!=i2){
            // save segment, keep 2 extra points at the end (no exact line cutting)
            if (i1!=seg->begin()) --i1;
            ml.push_back(dLine(i1,i2+1));
          }
          i2++; i1=i2;
          continue;
        }
        else {
          i2++; continue;
        }
      }

      if (i1==seg->begin()) { ++seg; continue; } // no modification
      if (i1!=i2) ml.push_back(dLine(i1,i2));
      seg = obj.erase(seg);
      mod = true;
      if (ml.size()) seg = obj.insert(seg, ml.begin(), ml.end()) + ml.size();
    }
    if (mod){
      if (obj.size()==0) vmap.del(i);
      else (vmap.put(i, obj));
    }
  }

}
