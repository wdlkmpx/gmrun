/*****************************************************************************
 *  $Id: gtkcompletionline.cc,v 1.33 2003/11/16 10:55:07 andreas99 Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"

using namespace std;

#include "config_prefs.h"
#include "gtkcompletionline.h"

#define HISTORY_FILE ".gmrun_history"

static int on_cursor_changed_handler = 0;
static int on_key_press_handler = 0;
static guint timeout_id = 0;

/* history search - backwards / fordwards -- see history.c */
const char * (*history_search_first_func) (HistoryFile *);
const char * (*history_search_next_func)  (HistoryFile *);
static gboolean searching_history = FALSE;

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
	EXT_HANDLER,
	CANCEL,
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
static int g_show_dot_files;

/* callbacks */
static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data);
static gboolean
on_scroll(GtkCompletionLine *cl, GdkEventScroll *event, gpointer data);

// https://developer.gnome.org/gobject/stable/gobject-Type-Information.html#G-DEFINE-TYPE-EXTENDED:CAPS
G_DEFINE_TYPE_EXTENDED (GtkCompletionLine,   /* type name */
                        gtk_completion_line, /* type name in lowercase, separated by '_' */
                        GTK_TYPE_ENTRY,      /* GType of the parent type */
                        (GTypeFlags)0,
                        NULL);
static void gtk_completion_line_dispose (GObject *object);
static void gtk_completion_line_finalize (GObject *object);
// see also https://developer.gnome.org/gobject/stable/GTypeModule.html#G-DEFINE-DYNAMIC-TYPE:CAPS

/* class_init */
static void gtk_completion_line_class_init (GtkCompletionLineClass *klass)
{
	GtkWidgetClass * object_class = GTK_WIDGET_CLASS (klass);
	guint s;

	GObjectClass * basic_class = G_OBJECT_CLASS (klass);
	basic_class->dispose  = gtk_completion_line_dispose;
	basic_class->finalize = gtk_completion_line_finalize;

	s = g_signal_new ("unique",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, unique),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[UNIQUE] = s;

	s = g_signal_new ("notunique",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, notunique),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[NOTUNIQUE] = s;

	s = g_signal_new ("incomplete",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, incomplete),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[INCOMPLETE] = s;

	s = g_signal_new ("runwithterm",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, runwithterm),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[RUNWITHTERM] = s;

	s = g_signal_new ("search_mode",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, search_mode),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[SEARCH_MODE] = s;

	s = g_signal_new ("search_not_found",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, search_not_found),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[SEARCH_NOT_FOUND] = s;

	s = g_signal_new ("search_letter",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, search_letter),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__VOID,
	                  G_TYPE_NONE, /* return_type */
	                  0);          /* n_params */
	gtk_completion_line_signals[SEARCH_LETTER] = s;

	s = g_signal_new ("ext_handler",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, ext_handler),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__POINTER,
	                  G_TYPE_NONE, /* return_type */
	                  1,           /* n_params */
	                   G_TYPE_POINTER);
	gtk_completion_line_signals[EXT_HANDLER] = s;

	s = g_signal_new ("cancel",
	                  G_TYPE_FROM_CLASS (object_class),
	                  G_SIGNAL_RUN_FIRST,
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, cancel),
	                  NULL,
	                  NULL,
	                  g_cclosure_marshal_VOID__POINTER,
	                  G_TYPE_NONE, /* return_type */
	                  1,           /* n_params */
	                  G_TYPE_POINTER);
	gtk_completion_line_signals[CANCEL] = s;

	klass->unique = NULL;
	klass->notunique = NULL;
	klass->incomplete = NULL;
	klass->runwithterm = NULL;
	klass->search_mode = NULL;
	klass->search_letter = NULL;
	klass->search_not_found = NULL;
	klass->ext_handler = NULL;
	klass->cancel = NULL;
}

/* init */
static void gtk_completion_line_init (GtkCompletionLine *self)
{
	/* Add object initialization / creation stuff here */
	self->where = NULL;
	self->cmpl = NULL;
	self->win_compl = NULL;
	self->list_compl = NULL;
	self->sort_list_compl = NULL;
	self->tree_compl = NULL;
	self->hist_search_mode = FALSE;
	self->hist_word[0] = 0;
	self->hist_word_count = 0;
	self->tabtimeout = 0;
	self->show_dot_files = 0;

	// required for gtk3+
	gtk_widget_add_events(GTK_WIDGET(self), GDK_SCROLL_MASK);

	on_key_press_handler = g_signal_connect(G_OBJECT(self),
	                                        "key_press_event",
	                                        G_CALLBACK(on_key_press), NULL);
	g_signal_connect(G_OBJECT(self), "scroll-event",
	                 G_CALLBACK(on_scroll), NULL);

	char * HOME = getenv ("HOME");
	char history_file[512] = "";
	if (HOME) {
		snprintf (history_file, sizeof (history_file), "%s/%s", HOME, HISTORY_FILE);
	}

	int HIST_MAX_SIZE;
	if (!config_get_int ("History", &HIST_MAX_SIZE))
		HIST_MAX_SIZE = 20;

	self->hist = history_new (history_file, HIST_MAX_SIZE);
	// hacks for prev/next will be applied
	history_unset_current (self->hist);

	self->first_key = 1;
}

static void gtk_completion_line_dispose (GObject *object)
{
	// GTK3: Pango-CRITICAL **: pango_layout_get_cursor_pos: assertion 'index >= 0 && index <= layout->length' failed
	// -- for some reason there's an error when the object is destroyed
	// -- The GtkCompletionLine 'cancel' signal makes gmrun destroy the object and exit
	// -- The current fix is to set an empty text
	gtk_entry_set_text (GTK_ENTRY (object), "");
	// --
	G_OBJECT_CLASS (gtk_completion_line_parent_class)->dispose (object);
}

static void gtk_completion_line_finalize (GObject *object)
{
	GtkCompletionLine * self = GTK_COMPLETION_LINE (object);
	if (self->hist) {
		                          // 0 = save | 1 = if changed
		history_save (self->hist, HISTORY_SAVE_IF_CHANGED);
		//history_print (self->hist); //debug
		history_destroy (self->hist);
		self->hist = NULL;
	}
	G_OBJECT_CLASS (gtk_completion_line_parent_class)->finalize (object);
}

// ====================================================================

void gtk_completion_line_last_history_item (GtkCompletionLine* object) {
	const char *last_item = history_last (object->hist);
	if (last_item) {
		gtk_entry_set_text (GTK_ENTRY(object), last_item);
	}
}

static void get_token(istream& is, string& s)
{
	s.clear();
	bool escaped = false;
	while (!is.eof()) {
		char c = is.get();
		if (is.eof())
			break;
		if (escaped) {
			s += c;
			escaped = false;
		} else if (c == '\\') {
			// s += c;
			escaped = true;
		} else if (::isspace(c)) {
			while (::isspace(c) && !is.eof()) c = is.get();
			if (!is.eof())
				is.unget();
			break;
		} else {
			s += c;
		}
	}
}

int get_words(GtkCompletionLine *object, vector<string>& words)
{
	string content(gtk_entry_get_text(GTK_ENTRY(object)));
	int pos_in_text = gtk_editable_get_position(GTK_EDITABLE(object));
	int pos = 0;
	{
		string::iterator i = content.begin() + pos_in_text;
		if (i != content.end())
			content.insert(i, ' ');
	}
	istringstream ss(content);

	while (!ss.eof()) {
		string s;
		// ss >> s;
		get_token(ss, s);
		words.push_back(s);
		if (ss.eof()) break;
		if (ss.tellg() < pos_in_text && ss.tellg() >= 0)
		++pos;
	}

	return pos;
}

int set_words(GtkCompletionLine *object, const vector<string>& words, int pos = -1)
{
	ostringstream ss;
	if (pos == -1)
		pos = words.size() - 1;
	int cur = 0;

	vector<string>::const_iterator
	i		= words.begin(),
	i_end	= words.end();

	GRegex *regex = g_regex_new (" ", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
	while (i != i_end) {
		gchar *quoted = g_regex_replace_literal (
				regex, (*i++).c_str(), -1, 0, "\\ ", G_REGEX_MATCH_NOTEMPTY, NULL);
		ss << quoted;
		if (i != i_end)
			ss << ' ';
		if (!pos && !cur)
			cur = ss.tellp();
		else
			--pos;
		g_free(quoted);
	}
	ss << ends;
	g_regex_unref (regex);

	if (words.size() == 1) {
		g_signal_emit_by_name (G_OBJECT(object), "ext_handler", words.back().c_str());
	}

	gtk_entry_set_text(GTK_ENTRY(object), 
				g_locale_to_utf8 (ss.str().c_str(), -1, NULL, NULL, NULL));
	gtk_editable_set_position(GTK_EDITABLE(object), cur);
	return cur;
}

static void generate_path()
{
	char *path_cstr = (char*)getenv("PATH");

	istringstream path_ss(path_cstr);
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
		if (tmp.length() != 0)
			path.insert(tmp);
	}
}

static int select_executables_only(const struct dirent* dent)
{
	int len = strlen(dent->d_name);
	int lenp = prefix.length();

	if (dent->d_name[0] == '.') {
		if (!g_show_dot_files)
			return 0;
		if (dent->d_name[1] == '\0')
			return 0;
		if ((dent->d_name[1] == '.') && (dent->d_name[2] == '\0'))
			return 0;
	}
	if (dent->d_name[len - 1] == '~')
		return 0;
	if (lenp == 0)
		return 1;
	if (lenp > len)
		return 0;

	if (strncmp(dent->d_name, prefix.c_str(), lenp) == 0)
		return 1;

	return 0;
}

#if defined(__GLIBC__) && __GLIBC_PREREQ(2, 10)
int my_alphasort (const struct dirent **a, const struct dirent **b)
#else
int my_alphasort (const void *_a, const void *_b)
#endif
{
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 10
	dirent const * const *a = (dirent const * const *)_a;
	dirent const * const *b = (dirent const * const *)_b;
#endif

	const char* s1 = (*a)->d_name;
	const char* s2 = (*b)->d_name;

	int l1 = strlen(s1);
	int l2 = strlen(s2);
	int result = strcmp(s1, s2);

	if (result == 0) return 0;

	if (l1 < l2) {
		int res2 = strncmp(s1, s2, l1);
		if (res2 == 0) return -1;
	} else {
		int res2 = strncmp(s1, s2, l2);
		if (res2 == 0) return 1;
	}

	return result;
}

static void generate_execs()
{
	execs.clear();

	for (StrSet::iterator i = path.begin(); i != path.end(); i++) {
		struct dirent **eps;
		int n = scandir(i->c_str(), &eps, select_executables_only, my_alphasort);
		if (n >= 0) {
			for (int j = 0; j < n; j++) {
				execs.insert(eps[j]->d_name);
				free(eps[j]);
			}
			free(eps);
		}
	}
}

static int generate_completion_from_execs(GtkCompletionLine *object)
{
	g_list_foreach(object->cmpl, (GFunc)g_string_free, NULL);
	g_list_free(object->cmpl);
	object->cmpl = NULL;

	for (StrSet::const_iterator i = execs.begin(); i != execs.end(); i++) {
		GString *the_fucking_gstring = g_string_new(i->c_str());
		object->cmpl = g_list_append(object->cmpl, the_fucking_gstring);
	}

	return 0;
}

static string get_common_part(const char *p1, const char *p2)
{
	string ret;

	while (*p1 == *p2 && *p1 != '\0' && *p2 != '\0') {
		ret += *p1;
		p1++;
		p2++;
	}

	return ret;
}

static int complete_common(GtkCompletionLine *object)
{
	GList *l;
	GList *ls = object->cmpl;
	vector<string> words;
	int pos = get_words(object, words);
	words[pos] = ((GString*)ls->data)->str;

	ls = g_list_next(ls);
	while (ls != NULL) {
		words[pos] = get_common_part(words[pos].c_str(),
                                 ((GString*)ls->data)->str);
		ls = g_list_next(ls);
	}

	set_words(object, words, pos);

	ls = object->cmpl;
	l = ls;
	return 0;
}

static int generate_dirlist(const char *what)
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
			if (!dir) {
				free(str);
				return GEN_CANT_COMPLETE;
			}
			closedir(dir);
			filename = p;
		}
		++p;
	}

	*filename = '\0';
	filename++;
	dest = str;
	dest += '/';

	dirlist.clear();
	struct dirent **eps;
	prefix = filename;
	n = scandir(dest.c_str(), &eps, select_executables_only, my_alphasort);
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
}

static int generate_completion_from_dirlist(GtkCompletionLine *object)
{
	g_list_foreach(object->cmpl, (GFunc)g_string_free, NULL);
	g_list_free(object->cmpl);
	object->cmpl = NULL;

	for (StrSet::const_iterator i = dirlist.begin(); i != dirlist.end(); i++) {
		GString *the_fucking_gstring = g_string_new(i->c_str());
		object->cmpl = g_list_append(object->cmpl, the_fucking_gstring);
	}

	return 0;
}

/* Expand tilde */
static int parse_tilda(GtkCompletionLine *object) {
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(object));
	const gchar *match = g_strstr_len(text, -1, "~");
	if (match) {
		gint cur = match - text;
		if ((cur > 0) && (text[cur - 1] != ' '))
			return 0;
		if ((guint)cur < strlen(text) - 1 && text[cur + 1] != '/') {
			// FIXME: Parse another user's home
			// #include <pwd.h>
			// struct passwd *p;
			// p=getpwnam(username);
			// printf("%s\n", p->pw_dir);
		} else {
			gtk_editable_insert_text(GTK_EDITABLE(object),
			g_get_home_dir(), strlen(g_get_home_dir()), &cur);
			gtk_editable_delete_text(GTK_EDITABLE(object), cur, cur + 1);
		}
	}

	return 0;
}

static void complete_from_list(GtkCompletionLine *object)
{
	parse_tilda(object);
	vector<string> words;
	int pos = get_words(object, words);

	prefix = words[pos];

	/* Completion list is opened */
	if (object->win_compl != NULL) {
		int current_pos;
		gpointer data;
		gtk_tree_model_get(object->sort_list_compl, &(object->list_compl_it), 0, &data, -1);
		object->where=(GList *)data;
		words[pos] = ((GString*)object->where->data)->str;
		current_pos = set_words(object, words, pos);

		GtkTreePath *path = gtk_tree_model_get_path(object->sort_list_compl, &(object->list_compl_it));
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(object->tree_compl), path, NULL, FALSE);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(object->tree_compl), path, NULL, TRUE, 0.5, 0.5);
		gtk_tree_path_free(path);

		gtk_editable_select_region(GTK_EDITABLE(object), object->pos_in_text, current_pos);
	} else {
		words[pos] = ((GString*)object->where->data)->str;
		object->pos_in_text = gtk_editable_get_position(GTK_EDITABLE(object));
		int current_pos = set_words(object, words, pos);
		object->where = g_list_next(object->where);
	}
}

static void on_cursor_changed(GtkTreeView *tree, gpointer data) {
	GtkCompletionLine *object = GTK_COMPLETION_LINE(data);

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(object->tree_compl));
	gtk_tree_selection_get_selected (selection, &(object->sort_list_compl), &(object->list_compl_it));

	g_signal_handler_block(G_OBJECT(object->tree_compl),
                      on_cursor_changed_handler);
	complete_from_list(object);
	g_signal_handler_unblock(G_OBJECT(object->tree_compl),
                         on_cursor_changed_handler);
}

static void cell_data_func( GtkTreeViewColumn *col, GtkCellRenderer *renderer,
                         GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
	gpointer data;
	gchar *str;

	gtk_tree_model_get(model, iter, 0, &data, -1);
	str = ((GString*)(((GList *)data)->data))->str;
	g_object_set(renderer, "text", str, NULL);
}


static int complete_line(GtkCompletionLine *object)
{
	parse_tilda(object);
	vector<string> words;
	int pos = get_words(object, words);
	prefix = words[pos];

	g_show_dot_files = object->show_dot_files;
	if (prefix[0] != '/') {
		if (object->where == NULL) {
			generate_path();
			generate_execs();
			generate_completion_from_execs(object);
			object->where = NULL;
		}
	} else if (object->where == NULL) {
		generate_dirlist(prefix.c_str());
		generate_completion_from_dirlist(object);
		object->where = NULL;
	}

	if (object->cmpl != NULL) {
		complete_common(object);
		object->where = object->cmpl;
	}

	if (object->where != NULL) {
		if (object->win_compl != NULL) {
			gboolean valid = gtk_tree_model_iter_next(object->sort_list_compl, &(object->list_compl_it));
			if(!valid) 
				gtk_tree_model_get_iter_first(object->sort_list_compl, &(object->list_compl_it));
		}
		complete_from_list(object);
	}

	GList *ls = object->cmpl;

	if (g_list_length(ls) == 1) {
		g_signal_emit_by_name(G_OBJECT(object), "unique");
		return GEN_COMPLETION_OK;
	} else if (g_list_length(ls) == 0 || ls == NULL) {
		g_signal_emit_by_name(G_OBJECT(object), "incomplete");
		return GEN_CANT_COMPLETE;
	} else if (g_list_length(ls) >  1) {
		g_signal_emit_by_name(G_OBJECT(object), "notunique");

		vector<string> words;
		int pos = get_words(object, words);

		if (words[pos] == ((GString*)ls->data)->str) {

			if (object->win_compl == NULL) {
				object->win_compl = gtk_window_new(GTK_WINDOW_POPUP);
				gtk_widget_set_name(object->win_compl, "gmrun_completion_window");

				/* attemp to silence warning: Gtk-WARNING **: Allocating size to Window ...
				https://git.eclipse.org/c/platform/eclipse.platform.swt.git/commit/?id=61a598af4dfda586b27d87537bb2d015bd614ba1
				https://sources.debian.org/src/clutter-gtk/1.8.2-2/clutter-gtk/gtk-clutter-actor.c/?hl=325#L325
				https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=867427
				*/
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 20 && GTK_MINOR_VERSION < 24
				// not a proper fix
				GtkRequisition r, r2;
				gtk_widget_get_preferred_size (GTK_WIDGET (object->win_compl), &r, &r2);
				GtkAllocation wal = { 0, 0, r2.width, r.height };
				gtk_widget_size_allocate (GTK_WIDGET (object->win_compl), &wal);
#endif

				object->list_compl = gtk_list_store_new (1, G_TYPE_POINTER);
				object->sort_list_compl = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(object->list_compl));
				object->tree_compl = gtk_tree_view_new_with_model(GTK_TREE_MODEL(object->sort_list_compl));
				g_object_unref(object->list_compl);
				g_object_unref(object->sort_list_compl);
				GtkTreeViewColumn *col;
				GtkCellRenderer *renderer;
				col = gtk_tree_view_column_new();
				gtk_tree_view_append_column(GTK_TREE_VIEW(object->tree_compl), col);
				renderer = gtk_cell_renderer_text_new();
				gtk_tree_view_column_pack_start(col, renderer, TRUE);
				gtk_tree_view_column_set_cell_data_func(col, renderer, cell_data_func, NULL, NULL);
 
				GtkTreeSelection *selection;
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(object->tree_compl));
				gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
				gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(object->tree_compl), FALSE);

				on_cursor_changed_handler = g_signal_connect(GTK_TREE_VIEW(object->tree_compl), "cursor-changed",
												G_CALLBACK(on_cursor_changed), object);

				g_signal_handler_block(G_OBJECT(object->tree_compl),
								on_cursor_changed_handler);

				GList *p = ls;
				while (p) {
					GtkTreeIter it;
					gtk_list_store_append(object->list_compl, &it);
					gtk_list_store_set (object->list_compl, &it, 0, p, -1);
					p = g_list_next(p);
				}

				GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
				gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_OUT);
				gtk_widget_show(scroll);
				gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
										GTK_POLICY_NEVER,
										GTK_POLICY_AUTOMATIC);

				gtk_container_set_border_width(GTK_CONTAINER(object->tree_compl), 2);
				gtk_container_add(GTK_CONTAINER (scroll), object->tree_compl);
				gtk_container_add(GTK_CONTAINER (object->win_compl), scroll);

				GdkWindow *top = gtk_widget_get_parent_window(GTK_WIDGET(object));
				int x, y;
				gdk_window_get_position(top, &x, &y);
				GtkAllocation al;
				gtk_widget_get_allocation( GTK_WIDGET(object), &al );
				x += al.x;
				y += al.y + al.height;

				// gtk_widget_popup(object->win_compl, x, y);
				gtk_window_move(GTK_WINDOW(object->win_compl), x, y);
				gtk_widget_show_all(object->win_compl);

				gtk_tree_view_columns_autosize(GTK_TREE_VIEW(object->tree_compl));
				gtk_widget_get_allocation(object->tree_compl, &al);
				gtk_widget_set_size_request(scroll, al.width + 40, 150);

				gtk_tree_model_get_iter_first (object->sort_list_compl, &(object->list_compl_it));
				gtk_tree_selection_select_iter (selection, &(object->list_compl_it));

				g_signal_handler_unblock(G_OBJECT(object->tree_compl),
									on_cursor_changed_handler);
			}

			return GEN_COMPLETION_OK;
		}
		return GEN_NOT_UNIQUE;
	}

	return GEN_COMPLETION_OK;
}

GtkWidget *
gtk_completion_line_new()
{
	return GTK_WIDGET(g_object_new(gtk_completion_line_get_type(), NULL));
}

static void
up_history(GtkCompletionLine* cl)
{
	static int pause = 0;
	const char * text_up;
	if (pause == 1) {
		text_up = history_last (cl->hist);
		pause = 0;
	} else {
		text_up = history_prev (cl->hist);
		if (!text_up) {  // empty, set a flag, next time we'll get something
			pause = 1;
			text_up = "";
		}
	}
	if (text_up) {
		gtk_entry_set_text (GTK_ENTRY(cl), text_up);
	}
}

static void
down_history(GtkCompletionLine* cl)
{
	static int pause = 0;
	const char * text_down;
	if (pause == 1) {
		text_down = history_first (cl->hist);
		pause = 0;
	} else {
		text_down = history_next (cl->hist);
		if (!text_down) {  // empty, set a flag, next time we'll get something
			pause = 1;
			text_down = "";
		}
	}
	if (text_down) {
		gtk_entry_set_text (GTK_ENTRY(cl), text_down);
	}
}

static void search_off (GtkCompletionLine* cl)
{
	int pos = gtk_editable_get_position (GTK_EDITABLE(cl));
	cl->hist_search_mode = FALSE;
	g_signal_emit_by_name (G_OBJECT(cl), "search_mode");
	history_unset_current (cl->hist);
}


static int
search_history (GtkCompletionLine* cl)
{ // must only be called if cl->hist_search_mode = TRUE
	/* a key is pressed and added to cl->hist_word */
	searching_history = TRUE;
	if (cl->hist_word[0])
	{
		const char * history_current_item;
		const char * search_str;
		int search_str_len;
		history_current_item = history_search_first_func (cl->hist);
		search_str = cl->hist_word;
		search_str_len = strlen (search_str);

		while (true)
		{
			const char * s;
			s = strstr (history_current_item, search_str);
			if (s) {
				if (strncmp (history_current_item, search_str, search_str_len) == 0) {
					gtk_entry_set_text (GTK_ENTRY(cl), history_current_item);
					g_signal_emit_by_name (G_OBJECT(cl), "search_letter");
					return 1;
				}
			}
			history_current_item = history_search_next_func (cl->hist);
			if (history_current_item == NULL) {
				g_signal_emit_by_name (G_OBJECT(cl), "search_not_found");
				break;
			}
		}
	}
	g_signal_emit_by_name (G_OBJECT(cl), "search_letter");
	searching_history = FALSE;

	return 1;
}

static guint tab_pressed(GtkCompletionLine* cl)
{
	if (cl->hist_search_mode == TRUE)
		search_off(cl);
	complete_line(cl);
	timeout_id = 0;
	return FALSE;
}

static void
clear_selection(GtkCompletionLine* cl)
{
	int pos = gtk_editable_get_position(GTK_EDITABLE(cl));
	gtk_editable_select_region(GTK_EDITABLE(cl), pos, pos);
}

static gboolean
on_scroll(GtkCompletionLine *cl, GdkEventScroll *event, gpointer data)
{
	// https://developer.gnome.org/gdk2/stable/gdk2-Event-Structures.html#GdkEventScroll
	// https://developer.gnome.org/gdk3/stable/gdk3-Event-Structures.html#GdkEventScroll
	if (event->type != GDK_SCROLL) {
		return FALSE;
	}
	GdkScrollDirection direction;
	direction = event->direction;
	if (direction == GDK_SCROLL_UP) {
		if (cl->win_compl != NULL) {
			gboolean valid;
#if GTK_CHECK_VERSION(3, 0, 0)
			valid = gtk_tree_model_iter_previous(cl->sort_list_compl, &(cl->list_compl_it));
#else
			GtkTreePath * path = gtk_tree_model_get_path (cl->sort_list_compl, &(cl->list_compl_it));
			valid = gtk_tree_path_prev (path);
			if (valid) {
				gtk_tree_model_get_iter (cl->sort_list_compl, &(cl->list_compl_it), path);
			}
			gtk_tree_path_free (path);
#endif
			if(!valid) {
				int rowCount = gtk_tree_model_iter_n_children (cl->sort_list_compl, NULL);
				gtk_tree_model_iter_nth_child(cl->sort_list_compl, &(cl->list_compl_it), NULL, rowCount - 1);
			}
			complete_from_list(cl);
		} else {
			up_history(cl);
		}
		if (cl->hist_search_mode == TRUE) {
			search_off(cl);
		}
		return TRUE;
	} else if (direction == GDK_SCROLL_DOWN) {
		if (cl->win_compl != NULL) {
			gboolean valid = gtk_tree_model_iter_next(cl->sort_list_compl, &(cl->list_compl_it));
			if(!valid) {
				gtk_tree_model_get_iter_first(cl->sort_list_compl, &(cl->list_compl_it));
			}
			complete_from_list(cl);
		} else {
			down_history(cl);
		}
		if (cl->hist_search_mode == TRUE) {
			search_off(cl);
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data)
{ // https://developer.gnome.org/gtk2/stable/GtkWidget.html#GtkWidget-key-press-event
	int key = event->keyval;
	switch (key)
	{
		case GDK_KEY_Control_R:
		case GDK_KEY_Control_L:
		case GDK_KEY_Shift_R:
		case GDK_KEY_Shift_L:
		case GDK_KEY_Alt_R:
		case GDK_KEY_Alt_L:
			break;

		case GDK_KEY_Tab:
			if (timeout_id != 0) {
				g_source_remove(timeout_id);
				timeout_id = 0;
			}
			tab_pressed(cl);
			return TRUE; /* stop signal emission */

		case GDK_KEY_Up:
			if (cl->win_compl != NULL) {
				gboolean valid;
#if GTK_CHECK_VERSION(3, 0, 0)
				valid = gtk_tree_model_iter_previous(cl->sort_list_compl, &(cl->list_compl_it));
#else
				GtkTreePath * path = gtk_tree_model_get_path (cl->sort_list_compl, &(cl->list_compl_it));
				valid = gtk_tree_path_prev (path);
				if (valid) {
					gtk_tree_model_get_iter (cl->sort_list_compl, &(cl->list_compl_it), path);
				}
				gtk_tree_path_free (path);
#endif
				if(!valid) {
					int rowCount = gtk_tree_model_iter_n_children (cl->sort_list_compl, NULL);
					gtk_tree_model_iter_nth_child(cl->sort_list_compl, &(cl->list_compl_it), NULL, rowCount - 1);
				}
				complete_from_list(cl);
			} else {
				up_history(cl);
			}
			if (cl->hist_search_mode == TRUE) {
				search_off(cl);
			}
			return TRUE; /* stop signal emission */

		case GDK_KEY_space:
		{
			cl->first_key = 0;
			bool search = cl->hist_search_mode;
			if (search)
				search_off(cl);
			if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
				if (!search) {
					int pos = gtk_editable_get_position(GTK_EDITABLE(cl));
				}
			}
		}
		return FALSE;

		case GDK_KEY_Down:
			if (cl->win_compl != NULL) {
				gboolean valid = gtk_tree_model_iter_next(cl->sort_list_compl, &(cl->list_compl_it));
				if(!valid) {
					gtk_tree_model_get_iter_first(cl->sort_list_compl, &(cl->list_compl_it));
				}
				complete_from_list(cl);
			} else {
				down_history(cl);
			}
			if (cl->hist_search_mode == TRUE) {
				search_off(cl);
			}
			return TRUE; /* stop signal emission */

		case GDK_KEY_Return:
			if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			}
			if (event->state & GDK_CONTROL_MASK) {
				g_signal_emit_by_name(G_OBJECT(cl), "runwithterm");
			} else {
				g_signal_emit_by_name(G_OBJECT(cl), "activate");
			}
			return TRUE; /* stop signal emission */

		case GDK_KEY_S:
		case GDK_KEY_s:
		case GDK_KEY_R:
		case GDK_KEY_r:
			if (event->state & GDK_CONTROL_MASK) {
				if (searching_history == FALSE) {
					/* set proper funcs for forward/backward search */
					if (key == GDK_KEY_R || key == GDK_KEY_r) {  /* reverse - backward */
						history_search_first_func = history_last;
						history_search_next_func  = history_prev;
					} else { /* from start - forward */
						history_search_first_func = history_first;
						history_search_next_func  = history_next;
					}
				}
				if (cl->hist_search_mode == FALSE) {
					gtk_entry_set_text (GTK_ENTRY (cl), "");
					cl->hist_search_mode = TRUE;
					cl->hist_word[0] = 0;
					cl->hist_word_count = 0;
					g_signal_emit_by_name(G_OBJECT(cl), "search_mode");
				}
				return TRUE; /* stop signal emission */
			} else goto ordinary;

		case GDK_KEY_BackSpace:
			if (cl->hist_search_mode == TRUE) {
				if (cl->hist_word[0]) {
					cl->hist_word_count--;
					cl->hist_word[cl->hist_word_count] = 0;
					search_history(cl);
					g_signal_emit_by_name(G_OBJECT(cl), "search_letter");
				}
				return TRUE; /* stop signal emission */
			}
			return FALSE;

		case GDK_KEY_Home:
		case GDK_KEY_End:
			clear_selection(cl);
			goto ordinary;

		case GDK_KEY_Escape:
			if (cl->hist_search_mode == TRUE) {
				search_off(cl);
			} else if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			} else {
				// user cancelled
				g_signal_emit_by_name(G_OBJECT(cl), "cancel");
			}
			return TRUE; /* stop signal emission */

		ordinary:
		default:
			cl->first_key = 0;
			if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			}
			cl->where = NULL;
			if (cl->hist_search_mode == TRUE) {
				// https://developer.gnome.org/gdk2/stable/gdk2-Event-Structures.html#GdkEventKey
				if (event->state & GDK_CONTROL_MASK)
					return TRUE; /* stop signal emission */
				if (event->length > 0) {
					// event->string = char: 'c' 'd'
					cl->hist_word[cl->hist_word_count] = event->string[0];
					cl->hist_word_count++;
					if (cl->hist_word_count <= 1000) {
						search_history(cl);
					}
					return TRUE; /* stop signal emission */
				} else
					search_off(cl);
			}
			if (cl->tabtimeout != 0) {
				if (timeout_id != 0) {
					g_source_remove(timeout_id);
					timeout_id = 0;
				}
				if (::isprint(*event->string)) {
					timeout_id = g_timeout_add(cl->tabtimeout,
							GSourceFunc(tab_pressed), cl);
				}
			}
			break;
	} //switch
	return FALSE;
}

// Local Variables: ***
// mode: c++ ***
// c-basic-offset: 2 ***
// End: ***
