// -*- c++ -*-
// $Id: prefs.h,v 1.2 2001/05/05 22:11:17 mishoo Exp $

#ifndef __PREFS_H__
#define __PREFS_H__

#include "ci_string.h"
#include <map>

class Prefs
{
 public:
  typedef std::map<ci_string, std::string> CONFIG;
  typedef std::pair<ci_string, std::string> PAIR;

 private:
  CONFIG vals_;

  void init(const std::string& file_name);
  string process(const std::string& cmd);

 public:
  Prefs();
  ~Prefs();

  bool get_string(const std::string& key, std::string& val) const;
  bool get_int(const std::string& key, int& val) const;
};

extern Prefs configuration;

#endif /* __PREFS_H__ */
