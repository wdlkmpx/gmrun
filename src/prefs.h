/*****************************************************************************
 *  $Id: prefs.h,v 1.6 2002/08/16 10:30:18 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#ifndef __PREFS_H__
#define __PREFS_H__

#include "ci_string.h"
#include <map>
#include <list>

class Prefs
{
 public:
  typedef std::map<ci_string, std::string> CONFIG;
  typedef std::pair<ci_string, std::string> PAIR;

 private:
  CONFIG vals_;
  CONFIG exts_;

  bool init(const std::string& file_name);
  string process(const std::string& cmd) const;

 public:
  Prefs();
  ~Prefs();

  bool get_string(const std::string& key, std::string& val) const;
  bool get_int(const std::string& key, int& val) const;
  bool get_string_list(const std::string& ket, std::list<string>& val) const;
  bool get_ext_handler(const std::string& ext, std::string& val) const;
};

extern Prefs configuration;

#endif /* __PREFS_H__ */
