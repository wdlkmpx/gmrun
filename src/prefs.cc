/*****************************************************************************
 *  $Id: prefs.cc,v 1.9 2002/08/16 10:48:22 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <list>

#define GMRUNRC "gmrunrc"

using std::ifstream;
using std::getline;
using std::string;
using std::list;

#include "prefs.h"
#include "config.h"

Prefs configuration;

Prefs::Prefs()
{
  string file_name = "/etc/";
  file_name += GMRUNRC;
  init(file_name);

  file_name = getenv("HOME");
  if (!file_name.empty()) {
    string::iterator i = file_name.end() - 1;
    if (*i == '/') file_name.erase(i);
    file_name += "/.";
    file_name += GMRUNRC;
    init(file_name);
  }
}

Prefs::~Prefs()
{}

bool Prefs::get_string(const string& key, string& val) const
{
  CONFIG::const_iterator i;
  i = vals_.find(ci_string(key.c_str()));
  if (i != vals_.end()) {
    val = process(i->second);
    return true;
  } else {
    return false;
  }
}

bool Prefs::get_ext_handler(const std::string& ext, std::string& val) const
{
  CONFIG::const_iterator i;
  i = exts_.find(ci_string(ext.c_str()));
  if (i != exts_.end()) {
    val = process(i->second);
    return true;
  } else {
    return false;
  }
}

bool Prefs::get_int(const std::string& key, int& val) const
{
  string sval;
  if (get_string(key, sval)) {
    int ret;
    if (sscanf(sval.c_str(), "%d", &ret) == 1) {
      val = ret;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool Prefs::init(const string& file_name)
{
  ifstream f(file_name.c_str());

  if (!f.good() || f.eof()) return false;

  while (f.good() && !f.eof()) {
    string line;
    char key[0x100];
    char val[0x100];

    getline(f, line);
    if (f.eof()) break;
    string::size_type i = line.find_first_not_of(" \t");
    if (i == string::npos) continue;
    if (line[i] == '#') continue;

    sscanf(line.c_str(), "%255[a-zA-Z_0-9:,;] = %255[^\n]", key, val);
    if (strncmp(key, "EXT:", 4) == 0) {
      string k(key + 4);
      size_t i = 0;
      while (i != string::npos) {
        string ext;
        size_t j = k.find(',', i);
        if (j != string::npos) {
          ext = k.substr(i, j - i);
          i = j + 1;
        } else {
          ext = k.substr(i);
          i = string::npos;
        }
        if (ext.length() > 0) {
#ifdef DEBUG
          std::cerr << "Extension: " << ext << " - " << val << std::endl;
#endif
          exts_[ext.c_str()] = val;
        }
      }
    } else {
      vals_[key] = val;
    }

#ifdef DEBUG
    std::cerr << "Key: " << key << ", val: " << vals_[key] << std::endl;
#endif
  }

  return true;
}

string Prefs::process(const std::string& cmd) const
{
  string::size_type i = cmd.find("${");
  string::size_type j = cmd.find("}", i);

  if (i == string::npos || j == string::npos) {
    return cmd;
  }

  string val;
  if (!get_string(cmd.substr(i + 2, j - i - 2), val)) return cmd;
  string ret(cmd);
  ret.replace(i, j - i + 1, val);
  return process(ret);
}

bool Prefs::get_string_list(const string& key, list<string>& val) const
{
  string sval;
  if (get_string(key, sval)) {
    const char *q, *p = sval.c_str();
    for (q = sval.c_str(); *q; ++q) {
      if (::isspace(*q)) {
        string s(p, q - p);
        val.push_back(s);
        while (*q && ::isspace(*q)) ++q;
        p = q;
      }
    }
    if (p != q) {
      string s(p, q - p);
      val.push_back(s);
    }
    return (val.size() > 0);
  }
  return false;
}

