/*****************************************************************************
 *  $Id: history.cc,v 1.10 2002/08/17 13:19:31 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#include <glib.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

#include "history.h"
#include "prefs.h"

HistoryFile::HistoryFile()
{
  m_file_entries = 0;
  m_filename = g_get_home_dir();
  m_filename += "/.gmrun_history";
  m_current = 0;
  m_default_set = false;
  read_the_file();
}

HistoryFile::~HistoryFile()
{}

void
HistoryFile::read_the_file()
{
  const char *filename = m_filename.c_str();
  ifstream f(filename);
  if (!f) return;

  while (!f.eof()) {
    char line_text[256];
    string line_str;

    f.getline(line_text, sizeof(line_text));
    if (*line_text) {
      line_str = line_text;
      history.push_back(line_str);
    }
  }

  m_file_entries = history.size();
  m_current = m_file_entries;
}

void
HistoryFile::sync_the_file()
{
  const char *filename = m_filename.c_str();

  int HIST_MAX_SIZE;

  if (!configuration.get_int("History", HIST_MAX_SIZE))
    HIST_MAX_SIZE = 20;

  ofstream f(filename, ios::out);

  int start = 0;
  if (history.size() > (size_t)HIST_MAX_SIZE)
    start = history.size() - HIST_MAX_SIZE;

  for (size_t i = start; i < history.size(); i++)
    if (history[i].length() != 0)
      f << history[i] << endl;

  f.flush();
}

void
HistoryFile::append(const char *entry)
{
  std::string ent = std::string(entry);
  if (!history.empty()) {
    StrArray::reverse_iterator i;
#ifdef DEBUG
    for_each(history.begin(), history.end(), DumpString(cerr));
#endif
    i = find(history.rbegin(), history.rend(), ent);
    if (i != history.rend()) {
#ifdef DEBUG
      cerr << "erasing "<< ent << endl;
#endif
      history.erase(remove(history.begin(), history.end(), ent));
    }
  }
  history.push_back(ent);
}

void
HistoryFile::set_default(const char *defstr)
{
  if (!m_default_set) {
    m_default = defstr;
    m_default_set = true;
  }
}

const char *
HistoryFile::operator [] (size_t index)
{
  if (index < 0 || index >= history.size()) {
    return m_default.c_str();
  }

  return history[index].c_str();
}

const char *
HistoryFile::prev()
{
  const char *ret = (*this)[--m_current];
  if (m_current < 0) m_current = history.size();
  return ret;
}

const char *
HistoryFile::next()
{
  const char *ret = (*this)[++m_current];
  if (m_current >= history.size()) m_current = -1;
  return ret;
}

const char *
HistoryFile::prev_to_first()
{
  if (m_current > 0) {
    return (*this)[--m_current];
  } else {
    return NULL;
  }
}

const char *
HistoryFile::next_to_last()
{
  if (m_current < history.size()) {
    return (*this)[++m_current];
  } else {
    return NULL;
  }
}

void
HistoryFile::clear_default()
{
  m_default_set = false;
}

void
HistoryFile::reset_position()
{
  m_current = m_file_entries;
}
