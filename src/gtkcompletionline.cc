/*****************************************************************************
 *  $Id: gtkcompletionline.cc,v 1.15 2001/07/17 16:23:33 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#include "gtkcompletionline.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkscrolledwindow.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <set>
#include <strstream>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

static int on_row_selected_handler = 0;
static int on_key_press_handler = 0;

#ifdef DEBUG
#define DEBUG_VAR(x) cerr << "-v- " << #x << " = " << x << endl
#define DEBUG_MSG(x) cerr << x << endl
#define DEBUG_FNC cerr << "-f- " << __PRETTY_FUNCTION__ << endl
#else
#define DEBUG_VAR
#define DEBUG_MSG
#define DEBUG_FNC
#endif

/* GLOBALS */

/* signals */
enum {
  UNIQUE,
  NOTUNIQUE,
  INCOMPLETE,
  RUNWITHTERM,
  SEARCH_MODE,
  SEARCH_LETTER,
  SEARCH_NOT_FOUND,
  LAST_SIGNAL
};

#define GEN_COMPLETION_OK  1
#define GEN_CANT_COMPLETE  2
#define GEN_NOT_UNIQUE     3

static guint gtk_completion_line_signals[LAST_SIGNAL];

typedef set<string> StrSet;

static StrSet path;
static StrSet execs;
static StrSet dirlist;
static string prefix;

/* callbacks */
static void gtk_completion_line_class_init(GtkCompletionLineClass *klass);
static void gtk_completion_line_init(GtkCompletionLine *object);

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data);

/* get_type */
guint gtk_completion_line_get_type(void)
{
  DEBUG_FNC;

  static guint type = 0;
  if (type == 0)
  {
    GtkTypeInfo type_info =
    {
      "GtkCompletionLine",
      sizeof(GtkCompletionLine),
      sizeof(GtkCompletionLineClass),
      (GtkClassInitFunc)gtk_completion_line_class_init,
      (GtkObjectInitFunc)gtk_completion_line_init,
      /*(GtkArgSetFunc)*/NULL /* reserved */,
      /*(GtkArgGetFunc)*/NULL /* reserved */
    };
    type = gtk_type_unique(gtk_entry_get_type(), &type_info);
  }
  return type;
}

/* class_init */
static void
gtk_completion_line_class_init(GtkCompletionLineClass *klass)
{
  DEBUG_FNC;

  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*)klass;

  gtk_completion_line_signals[UNIQUE] =
    gtk_signal_new("unique",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     unique),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[NOTUNIQUE] =
    gtk_signal_new("notunique",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     notunique),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[INCOMPLETE] =
    gtk_signal_new("incomplete",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     incomplete),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[RUNWITHTERM] =
    gtk_signal_new("runwithterm",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     runwithterm),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[SEARCH_MODE] =
    gtk_signal_new("search_mode",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     search_mode),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[SEARCH_NOT_FOUND] =
    gtk_signal_new("search_not_found",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     search_not_found),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_completion_line_signals[SEARCH_LETTER] =
    gtk_signal_new("search_letter",
                   GTK_RUN_FIRST, object_class->type,
                   GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                     search_letter),
                   gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals(object_class,
                               gtk_completion_line_signals, LAST_SIGNAL);

  klass->unique = NULL;
  klass->notunique = NULL;
  klass->incomplete = NULL;
  klass->runwithterm = NULL;
  klass->search_mode = NULL;
  klass->search_letter = NULL;
  klass->search_not_found = NULL;
}

/* init */
static void
gtk_completion_line_init(GtkCompletionLine *object)
{
  DEBUG_FNC;

  /* Add object initialization / creation stuff here */

  object->where = NULL;
  object->cmpl = NULL;
  object->win_compl = NULL;
  object->list_compl = NULL;
  object->hist_search_mode = GCL_SEARCH_OFF;
  object->hist_word = new string;

  on_key_press_handler =
    gtk_signal_connect(GTK_OBJECT(object), "key_press_event",
                       GTK_SIGNAL_FUNC(on_key_press), NULL);
  gtk_signal_connect(GTK_OBJECT(object), "key_release_event",
                     GTK_SIGNAL_FUNC(on_key_press), NULL);

  object->hist = new HistoryFile();
}

static int
get_words(GtkCompletionLine *object, vector<string>& words)
{
  DEBUG_FNC;

  string content(gtk_entry_get_text(GTK_ENTRY(object)));
  int pos_in_text = gtk_editable_get_position(GTK_EDITABLE(object));
  int pos = 0;
  {
    string::iterator i = content.begin() + pos_in_text;
    if (i != content.end())
      content.insert(i, ' ');
  }
  istrstream ss(content.c_str());
  DEBUG_VAR(pos_in_text);
  while (!ss.eof()) {
    string s;
    DEBUG_VAR(s);
    ss >> s;
    words.push_back(s);
    if (ss.eof()) break;
    DEBUG_VAR(ss.tellg());
    if (ss.tellg() < pos_in_text && ss.tellg() >= 0) {
      pos++;
      DEBUG_VAR(pos);
    }
  }
  DEBUG_MSG("*** END of function");
  return pos;
}

static int
set_words(GtkCompletionLine *object, const vector<string>& words, int pos = -1)
{
  DEBUG_FNC;

  strstream ss;
  bool first = true;
  if (pos == -1) {
    pos = words.size() - 1;
  }
  int where = 0;

  for (vector<string>::const_iterator i = words.begin();
       i != words.end(); i++) {
    if (first) {
      first = false;
    } else {
      ss << "  ";
    }
    ss << *i;
    if (pos == 0 && where == 0) {
      where = ss.tellp();
    } else {
      pos--;
    }
  }
  ss << ends;

  gtk_entry_set_text(GTK_ENTRY(object), ss.str());
  gtk_editable_set_position(GTK_EDITABLE(object), where);
  return where;
}

static void
generate_path()
{
  DEBUG_FNC;

  char *path_cstr = (char*)getenv("PATH");

  istrstream path_ss(path_cstr);
  string tmp;

  path.clear();
  while (!path_ss.eof()) {
    tmp = "";
    do {
      char c;
      c = path_ss.get();
      if (c == ':' || path_ss.eof()) break;
      else tmp += c;
    } while (true);
    if (tmp.length() != 0) path.insert(tmp);
  }
}

static int
select_executables_only(const struct dirent* dent)
{
  int len = strlen(dent->d_name);
  int lenp = prefix.length();
  struct stat stat_data;

  if (dent->d_name[0] == '.') {
    if (dent->d_name[1] == '\0') return 0;
    if (dent->d_name[1] == '.') {
      if (dent->d_name[2] == '\0') return 0;
    }
  }
  if (dent->d_name[len - 1] == '~') return 0;
  if (lenp == 0) return 1;
  if (lenp > len) return 0;

  if (strncmp(dent->d_name, prefix.c_str(), lenp) == 0) {
    return 1;
  }

  return 0;
}

static void
generate_execs()
{
  DEBUG_FNC;

  execs.clear();

  for (StrSet::iterator i = path.begin(); i != path.end(); i++) {
    struct dirent **eps;
    int n = scandir(i->c_str(), &eps, select_executables_only, alphasort);
    if (n >= 0) {
      for (int j = 0; j < n; j++) {
        execs.insert(eps[j]->d_name);
        free(eps[j]);
      }
      free(eps);
    }
  }
}

static int
generate_completion_from_execs(GtkCompletionLine *object)
{
  DEBUG_FNC;

  g_list_foreach(object->cmpl, (GFunc)g_string_free, NULL);
  g_list_free(object->cmpl);
  object->cmpl = NULL;

  for (StrSet::iterator i = execs.begin(); i != execs.end(); i++) {
    GString *the_fucking_gstring = g_string_new(i->c_str());
    object->cmpl = g_list_append(object->cmpl, the_fucking_gstring);
  }

  return 0;
}

static string
get_common_part(const char *s1, const char *s2)
{
  DEBUG_FNC;

  string ret;

  const char *p1 = s1;
  const char *p2 = s2;

  while (*p1 == *p2 && *p1 != '\0' && *p2 != '\0') {
    ret += *p1;
    p1++;
    p2++;
  }

  return ret;
}

static int
complete_common(GtkCompletionLine *object)
{
  DEBUG_FNC;

  GList *l;
  GList *ls = object->cmpl;
  vector<string> words;
  int pos = get_words(object, words);
  words[pos] = ((GString*)ls->data)->str;

  ls = ls->next;
  while (ls != NULL) {
    words[pos] = get_common_part(words[pos].c_str(),
                                 ((GString*)ls->data)->str);
    ls = ls->next;
  }

  set_words(object, words, pos);

  ls = object->cmpl;
  l = ls;

  if (words[pos] == ((GString*)(l->data))->str) {
    ls = g_list_remove_link(ls, l);
    ls = g_list_append(ls, l->data);
    g_list_free_1(l);
    object->cmpl = ls;
  }

  return 0;
}

static int
generate_dirlist(const char *what)
{
  DEBUG_FNC;

  char *str = strdup(what);
  char *p = str + 1;
  char *filename = str;
  string dest("/");
  int n;

  while (*p != '\0') {
    dest += *p;
    if (*p == '/') {
      DIR* dir = opendir(dest.c_str());
      if (!dir) goto dirty;
      closedir(dir);
      filename = p;
    }
    p++;
  }

  *filename = '\0';
  filename++;
  dest = str;
  dest += '/';

  dirlist.clear();
  struct dirent **eps;
  prefix = filename;
  n = scandir(dest.c_str(), &eps, select_executables_only, alphasort);
  if (n >= 0) {
    for (int j = 0; j < n; j++) {
      {
        string foo(dest);
        foo += eps[j]->d_name;
        struct stat filestatus;
        stat(foo.c_str(), &filestatus);
        if (S_ISDIR(filestatus.st_mode)) foo += '/';
        dirlist.insert(foo);
      }
      free(eps[j]);
    }
    free(eps);
  }

  free(str);
  return GEN_COMPLETION_OK;
 dirty:
  free(str);
  return GEN_CANT_COMPLETE;
}

static int
generate_completion_from_dirlist(GtkCompletionLine *object)
{
  DEBUG_FNC;

  g_list_foreach(object->cmpl, (GFunc)g_string_free, NULL);
  g_list_free(object->cmpl);
  object->cmpl = NULL;

  for (StrSet::iterator i = dirlist.begin(); i != dirlist.end(); i++) {
    GString *the_fucking_gstring = g_string_new(i->c_str());
    object->cmpl = g_list_append(object->cmpl, the_fucking_gstring);
  }

  return 0;
}

static int
parse_tilda(GtkCompletionLine *object)
{
  DEBUG_FNC;

  string text = gtk_entry_get_text(GTK_ENTRY(object));
  int where = text.find("~");
  if (where != string::npos) {
    if (where > 0) {
      if (text[where - 1] != ' ') return 0;
    }
    if (where < text.size() - 1 && text[where + 1] != '/') {
      // Parse user's home
    } else {
      text.replace(where, 1, g_get_home_dir());
      gtk_entry_set_text(GTK_ENTRY(object), text.c_str());
    }
  }

  return 0;
}

static void
complete_from_list(GtkCompletionLine *object)
{
  DEBUG_FNC;

  parse_tilda(object);
  vector<string> words;
  int pos = get_words(object, words);

  DEBUG_VAR(pos);
  DEBUG_VAR(words.size());
  prefix = words[pos];

  if (object->win_compl != NULL) {
    object->where = (GList*)gtk_clist_get_row_data(
      GTK_CLIST(object->list_compl), object->list_compl_items_where);

    words[pos] = ((GString*)object->where->data)->str;
    int current_pos = set_words(object, words, pos);
    gtk_entry_select_region(GTK_ENTRY(object),
                            object->pos_in_text, current_pos);

    int &item = object->list_compl_items_where;
    gtk_clist_select_row(GTK_CLIST(object->list_compl), item, 0);
    gtk_clist_moveto(GTK_CLIST(object->list_compl), item, 0, 0.5, 0.0);
  } else {
    words[pos] = ((GString*)object->where->data)->str;
    object->pos_in_text = gtk_editable_get_position(GTK_EDITABLE(object));
    int current_pos = set_words(object, words, pos);
    gtk_entry_select_region(GTK_ENTRY(object),
                            object->pos_in_text, current_pos);
    object->where = object->where->next;
    g_list_next(object->where);
  }
}

static int
on_row_selected(GtkWidget *ls, gint row, gint col, GdkEvent *ev, gpointer data)
{
  GtkCompletionLine *cl = GTK_COMPLETION_LINE(data);

  cl->list_compl_items_where = row;
  DEBUG_VAR(row);
  gtk_signal_handler_block(GTK_OBJECT(cl->list_compl),
                           on_row_selected_handler);
  complete_from_list(cl);
  gtk_signal_handler_unblock(GTK_OBJECT(cl->list_compl),
                             on_row_selected_handler);
}

static void
select_appropiate(GtkCompletionLine *object)
{
  for (int i = 0; i < object->list_compl_nr_rows; ++i) {
    char *text;
    gtk_clist_get_text(GTK_CLIST(object->list_compl), i, 0, &text);
    if (strncmp(prefix.c_str(), text, prefix.length())) {
      gtk_signal_handler_block(GTK_OBJECT(object->list_compl),
                               on_row_selected_handler);
      gtk_clist_select_row(GTK_CLIST(object->list_compl), i, 0);
      object->list_compl_items_where = i;
      gtk_signal_handler_unblock(GTK_OBJECT(object->list_compl),
                                 on_row_selected_handler);
      break;
    }
  }
}

static void
get_prefix(GtkCompletionLine *object)
{
  parse_tilda(object);
  vector<string> words;
  int pos = get_words(object, words);
  DEBUG_VAR(pos);
  DEBUG_VAR(words.size());
  prefix = words[pos];
  DEBUG_VAR(prefix);
}

static int
complete_line(GtkCompletionLine *object)
{
  DEBUG_FNC;

  parse_tilda(object);
  vector<string> words;
  int pos = get_words(object, words);
  DEBUG_VAR(pos);
  DEBUG_VAR(words.size());
  prefix = words[pos];

  if (prefix[0] != '/') {
    if (object->where == NULL) {
      generate_path();
      generate_execs();
      generate_completion_from_execs(object);
      object->where = NULL;
    }
  } else {
    if (object->where == NULL) {
      generate_dirlist(prefix.c_str());
      generate_completion_from_dirlist(object);
      object->where = NULL;
    }
  }

  if (object->cmpl != NULL) {
    complete_common(object);
    object->where = object->cmpl;
  }

  // FUCK C! C++ Rules!
  if (object->where != NULL) {
    if (object->win_compl != NULL) {
      int &item = object->list_compl_items_where;
      item++;
      if (item >= object->list_compl_nr_rows) {
        item = object->list_compl_nr_rows - 1;
      }
    }
    complete_from_list(object);
  } else if (object->cmpl != NULL) {
    complete_common(object);
    object->where = object->cmpl;
  }

  GList *ls = object->cmpl;

  if (g_list_length(ls) == 1) {
    gtk_signal_emit_by_name(GTK_OBJECT(object), "unique");
    return GEN_COMPLETION_OK;
  } else if (g_list_length(ls) == 0 || ls == NULL) {
    gtk_signal_emit_by_name(GTK_OBJECT(object), "incomplete");
    return GEN_CANT_COMPLETE;
  } else if (g_list_length(ls) >  1) {
    gtk_signal_emit_by_name(GTK_OBJECT(object), "notunique");

    vector<string> words;
    int pos = get_words(object, words);

    DEBUG_VAR(((GString*)ls->data)->str);
    DEBUG_VAR(words[pos]);

    if (words[pos] == ((GString*)ls->data)->str) {

      if (object->win_compl == NULL) {
        object->win_compl = gtk_window_new(GTK_WINDOW_POPUP);
        /*gtk_window_set_position(GTK_WINDOW(object->win_compl),
          GTK_WIN_POS_MOUSE);*/

        gtk_window_set_policy(GTK_WINDOW(object->win_compl),
                              FALSE, FALSE, TRUE);
        gtk_container_set_border_width(GTK_CONTAINER(object->win_compl), 5);

        object->list_compl = gtk_clist_new(1);

        on_row_selected_handler =
          gtk_signal_connect(GTK_OBJECT(object->list_compl), "select_row",
                             GTK_SIGNAL_FUNC(on_row_selected), object);

        gtk_signal_handler_block(GTK_OBJECT(object->list_compl),
                                 on_row_selected_handler);

        GList *p = ls;
        object->list_compl_nr_rows = 0;
        while (p) {
          char *tmp[2];
          tmp[0] = ((GString*)p->data)->str;
          tmp[1] = NULL;
          int row = gtk_clist_append(GTK_CLIST(object->list_compl), tmp);
          gtk_clist_set_row_data(GTK_CLIST(object->list_compl), row, p);
          object->list_compl_nr_rows++;
          p = g_list_next(p);
        }

        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_widget_show(scroll);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
                                       GTK_POLICY_NEVER,
                                       GTK_POLICY_AUTOMATIC);

        gtk_container_add(GTK_CONTAINER (scroll), object->list_compl);

        object->list_compl_items_where = 0;

        gtk_widget_show(object->list_compl);
        int w = gtk_clist_optimal_column_width(GTK_CLIST(object->list_compl),
                                               0);
        gtk_widget_set_usize(scroll, w + 40, 150);

        gtk_container_add(GTK_CONTAINER(object->win_compl), scroll);

        GdkWindow *top = gtk_widget_get_parent_window(GTK_WIDGET(object));
        int x, y;
        gdk_window_get_position(top, &x, &y);
        x += GTK_WIDGET(object)->allocation.x;
        y += GTK_WIDGET(object)->allocation.y +
          GTK_WIDGET(object)->allocation.height;

        gtk_widget_popup(object->win_compl, x, y);
        // gtk_widget_show(object->win_compl);

        gtk_clist_select_row(GTK_CLIST(object->list_compl),
                             object->list_compl_items_where, 0);

        gtk_signal_handler_unblock(GTK_OBJECT(object->list_compl),
                                   on_row_selected_handler);
      }

      return GEN_COMPLETION_OK;
    }
    return GEN_NOT_UNIQUE;
  }
}

GtkWidget *
gtk_completion_line_new()
{
  DEBUG_FNC;

  return GTK_WIDGET(gtk_type_new(gtk_completion_line_get_type()));
}

static void
up_history(GtkCompletionLine* cl)
{
  cl->hist->set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
  gtk_entry_set_text(GTK_ENTRY(cl), cl->hist->prev());
}

static void
down_history(GtkCompletionLine* cl)
{
  cl->hist->set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
  gtk_entry_set_text(GTK_ENTRY(cl), cl->hist->next());
}

static int
search_back_history(GtkCompletionLine* cl)
{
  if (!cl->hist_word->empty()) {
    const char * histext;
    while (true) {
      histext = cl->hist->prev_to_first();
      if (cl == NULL || histext == NULL) {
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_not_found");
        break;
      }
      string s = histext;
      string::size_type i;
      i = s.find(*cl->hist_word);
      if (i != string::npos) {
        gtk_entry_set_text(GTK_ENTRY(cl), histext);
        gtk_entry_select_region(GTK_ENTRY(cl),
                                i, i + cl->hist_word->length());
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_letter");
        return 1;
      }
    }
  }

  return 0;
}

static int
search_forward_history(GtkCompletionLine* cl)
{
  if (!cl->hist_word->empty()) {
    const char * histext;
    while (true) {
      histext = cl->hist->next_to_last();
      if (cl == NULL || histext == NULL) {
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_not_found");
        break;
      }
      string s = histext;
      string::size_type i;
      i = s.find(*cl->hist_word);
      if (i != string::npos) {
        gtk_entry_set_text(GTK_ENTRY(cl), histext);
        gtk_entry_select_region(GTK_ENTRY(cl),
                                i, i + cl->hist_word->length());
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_letter");
        return 1;
      }
    }
  }

  return 0;
}

static int
search_history(GtkCompletionLine* cl)
{
  switch (cl->hist_search_mode) {
   case GCL_SEARCH_REW:
    return search_back_history(cl);

   case GCL_SEARCH_FWD:
    return search_forward_history(cl);

   default:
    return -1;
  }
}

static int
inverse_search_history(GtkCompletionLine* cl)
{
  switch (cl->hist_search_mode) {
   case GCL_SEARCH_FWD:
    return search_back_history(cl);

   case GCL_SEARCH_REW:
    return search_forward_history(cl);

   default:
    return -1;
  }
}

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data)
{
  DEBUG_FNC;

#define STOP_PRESS \
  (gtk_signal_emit_stop_by_name(GTK_OBJECT(cl),   "key_press_event"))
#define STOP_RELEASE \
  (gtk_signal_emit_stop_by_name(GTK_OBJECT(cl), "key_release_event"))

  switch (event->type) {
   case GDK_KEY_PRESS:
    switch (event->keyval) {
     case GDK_Tab:
      complete_line(cl);
      STOP_PRESS;
      return TRUE;
     case GDK_Up:
      if (cl->win_compl != NULL) {
        int &item = cl->list_compl_items_where;
        item--;
        if (item < 0) {
          item = 0;
        } else {
          complete_from_list(cl);
        }
      } else {
        up_history(cl);
      }
      if (cl->hist_search_mode != GCL_SEARCH_OFF) {
        cl->hist_search_mode = GCL_SEARCH_OFF;
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_mode");
      }
      STOP_PRESS;
      return TRUE;
     case GDK_space:
     {
       int pos = gtk_editable_get_position(GTK_EDITABLE(cl));
       gtk_entry_select_region(GTK_ENTRY(cl), pos, pos);
       if (cl->win_compl != NULL) {
         gtk_widget_destroy(cl->win_compl);
         cl->win_compl = NULL;
       }
       return TRUE;
     }
     case GDK_Down:
      if (cl->win_compl != NULL) {
        int &item = cl->list_compl_items_where;
        item++;
        if (item >= cl->list_compl_nr_rows) {
          item = cl->list_compl_nr_rows - 1;
        } else {
          complete_from_list(cl);
        }
      } else {
        down_history(cl);
      }
      if (cl->hist_search_mode != GCL_SEARCH_OFF) {
        cl->hist_search_mode = GCL_SEARCH_OFF;
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_mode");
      }
      STOP_PRESS;
      return TRUE;
     case GDK_Return:
      if (cl->win_compl != NULL) {
        gtk_widget_destroy(cl->win_compl);
        cl->win_compl = NULL;
      }
      if (event->state & GDK_CONTROL_MASK) {
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "runwithterm");
      } else {
        gtk_signal_emit_by_name(GTK_OBJECT(cl), "activate");
      }
      STOP_PRESS;
      return TRUE;
     case GDK_R:
     case GDK_r:
      if (event->state & GDK_CONTROL_MASK) {
        if (cl->hist_search_mode != GCL_SEARCH_OFF) {
          search_back_history(cl);
        } else {
          cl->hist_search_mode = GCL_SEARCH_REW;
          cl->hist_word->clear();
          gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_mode");
        }
        STOP_PRESS;
        return TRUE;
      }
      return FALSE;
     case GDK_S:
     case GDK_s:
      if (event->state & GDK_CONTROL_MASK) {
        if (cl->hist_search_mode != GCL_SEARCH_OFF) {
          search_forward_history(cl);
        } else {
          cl->hist_search_mode = GCL_SEARCH_FWD;
          cl->hist_word->clear();
          gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_mode");
        }
        STOP_PRESS;
        return TRUE;
      }
      return FALSE;
     case GDK_BackSpace:
      if (cl->hist_search_mode != GCL_SEARCH_OFF) {
        if (!cl->hist_word->empty()) {
          cl->hist_word->erase(cl->hist_word->length() - 1);
          inverse_search_history(cl);
        }
        STOP_PRESS;
        return TRUE;
      }
      return FALSE;
     case GDK_Escape:
     case GDK_End:
     {
       int pos = gtk_editable_get_position(GTK_EDITABLE(cl));
       gtk_entry_select_region(GTK_ENTRY(cl), pos, pos);
       if (cl->win_compl != NULL) {
         gtk_widget_destroy(cl->win_compl);
         cl->win_compl = NULL;
         STOP_PRESS;
         return TRUE;
       }
       if (cl->hist_search_mode != GCL_SEARCH_OFF) {
         cl->hist_search_mode = GCL_SEARCH_OFF;
         gtk_signal_emit_by_name(GTK_OBJECT(cl), "search_mode");
         STOP_PRESS;
         return TRUE;
       }
       return FALSE;
     }
     default:
      if (cl->win_compl != NULL) {
        gtk_widget_destroy(cl->win_compl);
        cl->win_compl = NULL;
      }
      cl->where = NULL;
      if (cl->hist_search_mode != GCL_SEARCH_OFF &&
          event->keyval >= 32 && event->keyval <= 127) {
        *cl->hist_word += (char)(event->keyval);
        if (search_history(cl) <= 0) {
          cl->hist_word->erase(cl->hist_word->length() - 1);
        }
        STOP_PRESS;
        return TRUE;
      }
      break;
    }
    break;
   case GDK_KEY_RELEASE:
    switch (event->keyval) {
     default:
      break;
    }
    break;
  }
  return FALSE;

#undef STOP_PRESS
#undef STOP_RELEASE
}
