#ifndef LIB_H
#define LIB_H

#include "vmap2/vmap2.h"
#include "vmap2/vmap2io.h"
#include "vmap2/vmap2tools.h"


// make new reference object for a text object:
VMap2obj
make_ref_obj(const VMap2obj & objt, const std::string & src);

/*********************************************************************/
// Read source files into vmap (simple wripper around vmap2_import)

void read_src(VMap2 & vmap, const std::string & fname);

/*********************************************************************/
// Type configuration files

// basic type table, string -> string map, used in fi1, no1
std::map<std::string, std::string>
read_tconv_str(const std::string & fname);

// type table for fi2 -  additional action field
struct tconv_fi2_t {std::string action; std::string type;};
std::map<std::string, tconv_fi2_t>
read_tconv_fi2(const std::string & fname);

// normal text conf, used in fi1t, no1t
struct tconv_txt_t {
  std::string type; std::string rtype; int angle; std::string scale;};
std::map<std::string, tconv_txt_t>
read_tconv_txt(const std::string & fname);

// fi2t -- additional legend field
struct tconv_fi2t_t {
  std::string type; std::string rtype; int legend; int angle; std::string scale;};
std::map<std::string, tconv_fi2t_t>
read_tconv_fi2t(const std::string & fname);

// update_map -- object conversion table
struct oconv_t {
  std::string src, type, lat, lon, name_re, name_subst;};
std::list<oconv_t>
read_oconv(const std::string & fname);


/*********************************************************************/
// finctions for import_fi{1,2}

// extend objects beyond map bounaries
void extend_object(dMultiLine & crds, const dRect & box);

// check if string str contains suffix suff
bool check_suff(const std::string & str, const std::string & suff);

// change option name, repack double value
void opt_mv_dbl(Opt & opts, const std::string & src, const std::string & dst);

// change option name
void opt_mv_txt(Opt & opts, const std::string & src, const std::string & dst);

// delete option with a specific ("default") value
void opt_del_def(Opt & opts, const std::string & src, const std::string & def);

/*********************************************************************/

#endif
