/*****************************************************************************
 *  $Id: gtkcompletionline.cc,v 1.1 2001/02/23 07:48:28 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *
 *  Dedicated to Marius Ologesa (my main man).
 *****************************************************************************/


#include "gtkcompletionline.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <set>
#include <strstream>
#include <string>
#include <vector>
using namespace std;

/* GLOBALS */

/* signals */
enum {
	UNIQUE,
    NOTUNIQUE,
    INCOMPLETE,
    UPARROW,
    DNARROW,
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

    gtk_completion_line_signals[UPARROW] =
        gtk_signal_new("uparrow",
                       GTK_RUN_FIRST, object_class->type,
                       GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                         uparrow),
                       gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_completion_line_signals[DNARROW] =
        gtk_signal_new("dnarrow",
                       GTK_RUN_FIRST, object_class->type,
                       GTK_SIGNAL_OFFSET(GtkCompletionLineClass,
                                         dnarrow),
                       gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

    gtk_object_class_add_signals(object_class,
                                 gtk_completion_line_signals, LAST_SIGNAL);
    
    klass->unique = NULL;
    klass->notunique = NULL;
    klass->incomplete = NULL;
    klass->uparrow = NULL;
    klass->dnarrow = NULL;
}

/* init */
static void
gtk_completion_line_init(GtkCompletionLine *object)
{
	/* Add object initialization / creation stuff here */

    object->where = NULL;
    object->cmpl = NULL;

    gtk_signal_connect(GTK_OBJECT(object), "key_press_event",
                       GTK_SIGNAL_FUNC(on_key_press), NULL);
    gtk_signal_connect(GTK_OBJECT(object), "key_release_event",
                       GTK_SIGNAL_FUNC(on_key_press), NULL);
}

static int
get_words(GtkCompletionLine *object, vector<string>& words)
{
    istrstream ss(gtk_entry_get_text(GTK_ENTRY(object)));
    int pos_in_text = gtk_editable_get_position(GTK_EDITABLE(object));
    int pos = 0;
    while (!ss.eof()) {
        string s;
        ss >> s;
        words.push_back(s);
        if (ss.tellg() < pos_in_text) {
            pos++;
        }
    }
    return pos;
}

static void
set_words(GtkCompletionLine *object, vector<string>& words, int pos = -1)
{
    strstream ss;
    bool first = true;
    if (pos == -1) {
        pos = words.size() - 1;
    }
    int where = 0;

    for (vector<string>::iterator i = words.begin(); i != words.end(); i++) {
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
}

static void
generate_path()
{
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
    
    if (dent->d_name[0] == '.') return 0;
    if (dent->d_name[len - 1] == '~') return 0;
    if (lenp == 0) return 1;
    if (lenp > len) return 0;
    
    if (strncmp(dent->d_name, prefix.c_str(), lenp) == 0) return 1;
    
    return 0;
}

static void
generate_execs()
{
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
    string text = gtk_entry_get_text(GTK_ENTRY(object));
    int where = text.find("~");
    if (where >= 0 && where < text.length()) {
        text.replace(where, 1, g_get_home_dir());
        gtk_entry_set_text(GTK_ENTRY(object), text.c_str());
    }

    return 0;
}

static int
complete_line(GtkCompletionLine *object)
{
    parse_tilda(object);
    vector<string> words;
    int pos = get_words(object, words);
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
    // FUCK C! C++ Rules!
    if (object->where != NULL) {
        words[pos] = ((GString*)object->where->data)->str;
        set_words(object, words, pos);
        object->where = object->where->next;
        if (object->where == NULL) object->where = object->cmpl;
        // wouldn't be where++ better? :)
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
        if (strcmp(gtk_entry_get_text(GTK_ENTRY(object)),
                   ((GString*)ls->data)->str) == 0) {
            return GEN_COMPLETION_OK;
        }
        return GEN_NOT_UNIQUE;
    }
}

GtkWidget *
gtk_completion_line_new()
{
    return GTK_WIDGET(gtk_type_new(gtk_completion_line_get_type()));
}

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data)
{
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
            gtk_signal_emit_by_name(GTK_OBJECT(cl), "uparrow");
            STOP_PRESS;
            return TRUE;
          case GDK_Down:
            gtk_signal_emit_by_name(GTK_OBJECT(cl), "dnarrow");
            STOP_PRESS;
            return TRUE;
          case GDK_Return:
            gtk_signal_emit_by_name(GTK_OBJECT(cl), "activate");
            STOP_PRESS;
            return TRUE;
          default:
            cl->where = NULL;
            return FALSE;
        }
        break;
      case GDK_KEY_RELEASE:
        switch (event->keyval) {
        }
        break;
      default:
        break;
    }
    return FALSE;

#undef STOP_PRESS
#undef STOP_RELEASE
}
