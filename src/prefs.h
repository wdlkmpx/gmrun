// -*- c++ -*-
// $Id: prefs.h,v 1.1 2001/05/04 09:05:44 mishoo Exp $

#ifndef __PREFS_H__
#define __PREFS_H__

#include <map>
#include <string>

class Prefs
{
 public:
  typedef std::map<string, string> CONFIG;
  typedef std::pair<string, string> PAIR;

 private:
  CONFIG vals_;

  void init(const std::string& file_name);

 public:
  Prefs();
  ~Prefs();

  bool get_string(const std::string& key, std::string& val) const;
  bool get_int(const std::string& key, int& val) const;
};

extern Prefs configuration;

#endif /* __PREFS_H__ */
