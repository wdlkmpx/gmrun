// -*- c++ -*-
// $Id: prefs.cc,v 1.2 2001/05/04 09:24:51 mishoo Exp $

#include <string>
#include <fstream>
#include <iostream>

#include <stdio.h>

using std::string;
using std::ifstream;
using std::getline;

#include "prefs.h"

Prefs configuration;

Prefs::Prefs()
{
  string file_name = getenv("HOME");
  if (!file_name.empty()) {
    string::iterator i = file_name.end();
    if (*i == '/') file_name.erase(i);
    file_name += "/.gmrun_config";
    init(file_name);
  }
}

Prefs::~Prefs()
{}

bool Prefs::get_string(const string& key, string& val) const
{
  CONFIG::const_iterator i;
  i = vals_.find(key);
  if (i != vals_.end()) {
    val = i->second;
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

void Prefs::init(const string& file_name)
{
  ifstream f(file_name.c_str());

  while (f.good() && !f.eof()) {
    string line;
    char key[0x100];
    char val[0x100];

    getline(f, line);
    if (f.eof()) break;
    string::size_type i = line.find_first_not_of(" \t");
    if (i == string::npos) continue;
    if (line[i] == '#') continue;
    
    sscanf(line.c_str(), "%30[a-zA-Z_0-9] = %255[^\n]",
           key, val);
    vals_[key] = val;
  }
}
