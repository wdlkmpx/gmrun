/*****************************************************************************
 *  $Id: history.h,v 1.5 2001/07/18 07:03:39 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#ifndef __HISTORY_H__
#define __HISTORY_H__

#include <vector>
#include <string>
using namespace std;

class HistoryFile
{
 protected:
  int m_file_entries;
  string m_filename;
  string m_default;
  bool m_default_set;
  int m_current;
	
  typedef vector<string> StrArray;
  StrArray history;
	
 public:
  HistoryFile();
  ~HistoryFile();
	
  void append(const char *entry);
  void set_default(const char *defstr);
  void clear_default();

  void reset_position();
	
  const char * operator [] (int index);
	
  const char * prev();
  const char * next();

  const char * prev_to_first();
  const char * next_to_last();
	
 protected:
  void read_the_file();
  void sync_the_file();
};

#endif // __HISTORY_H__
