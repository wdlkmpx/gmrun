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

#include "gtkcompletionline.h"

static int on_cursor_changed_handler = 0;
static int on_key_press_handler = 0;

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
typedef vector<string> StrList;

static StrSet path;
static StrSet execs;
static StrSet dirlist;
static string prefix;
static int g_show_dot_files;

/* callbacks */
static void gtk_completion_line_class_init(GtkCompletionLineClass *klass);
static void gtk_completion_line_init(GtkCompletionLine *object);

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data);
static gboolean
on_scroll(GtkCompletionLine *cl, GdkEventScroll *event, gpointer data);

/* get_type */
GType gtk_completion_line_get_type(void)
{
	static GType type = 0;
	if (type == 0)
	{
		static const GTypeInfo type_info =
		{
			sizeof(GtkCompletionLineClass),
			NULL,
			NULL,
			(GClassInitFunc)gtk_completion_line_class_init,
			NULL,
			NULL,
			sizeof(GtkCompletionLine),
			0,
			(GInstanceInitFunc)gtk_completion_line_init,
			NULL
		};
		type = g_type_register_static(GTK_TYPE_ENTRY, "GtkCompletionLine",
                                  &type_info, (GTypeFlags)0);
	}
	return type;
}

/* class_init */
static void
gtk_completion_line_class_init(GtkCompletionLineClass *klass)
{
	GtkWidgetClass *object_class;
	guint s;

	object_class = (GtkWidgetClass*)klass;

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
	                  G_STRUCT_OFFSET (GtkCompletionLineClass, ext_handler),
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
static void gtk_completion_line_init(GtkCompletionLine *object)
{
	/* Add object initialization / creation stuff here */
	object->where = NULL;
	object->cmpl = NULL;
	object->win_compl = NULL;
	object->list_compl = NULL;
	object->sort_list_compl = NULL;
	object->tree_compl = NULL;
	object->hist_search_mode = GCL_SEARCH_OFF;
	object->hist_word = new string;
	object->tabtimeout = 0;
	object->show_dot_files = 0;

	// required for gtk3+
	gtk_widget_add_events(GTK_WIDGET(object), GDK_SCROLL_MASK);

	on_key_press_handler = g_signal_connect(GTK_WIDGET(object),
	                                        "key_press_event",
	                                        G_CALLBACK(on_key_press), NULL);
	g_signal_connect(GTK_WIDGET(object), "scroll-event",
	                 G_CALLBACK(on_scroll), NULL);

	object->hist = new HistoryFile();

	object->first_key = 1;
}

void gtk_completion_line_last_history_item(GtkCompletionLine* object) {
	const char *last_item = object->hist->last_item();
	if (last_item) {
		object->hist->set_default("");
		const char* txt = object->hist->prev();
		gtk_entry_set_text(GTK_ENTRY(object),
				g_locale_to_utf8 (txt, -1, NULL, NULL, NULL));
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
		const string& s = words.back();
		size_t pos = s.rfind('.');
		if (pos != string::npos)
			g_signal_emit_by_name(GTK_WIDGET(object), "ext_handler", s.substr(pos + 1).c_str());
		else
			g_signal_emit_by_name(GTK_WIDGET(object), "ext_handler", NULL);
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
		g_signal_emit_by_name(GTK_WIDGET(object), "unique");
		return GEN_COMPLETION_OK;
	} else if (g_list_length(ls) == 0 || ls == NULL) {
		g_signal_emit_by_name(GTK_WIDGET(object), "incomplete");
		return GEN_CANT_COMPLETE;
	} else if (g_list_length(ls) >  1) {
		g_signal_emit_by_name(GTK_WIDGET(object), "notunique");

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
	cl->hist->set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
	gtk_entry_set_text(GTK_ENTRY(cl), 
				g_locale_to_utf8 (cl->hist->prev(), -1, NULL, NULL, NULL));
}

static void
down_history(GtkCompletionLine* cl)
{
	cl->hist->set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
	gtk_entry_set_text(GTK_ENTRY(cl), 
				g_locale_to_utf8 (cl->hist->next(), -1, NULL, NULL, NULL));
}

static int
search_back_history(GtkCompletionLine* cl, bool avance, bool begin)
{
	if (!cl->hist_word->empty()) {
		const char * histext;
		if (avance) {
			histext = cl->hist->prev_to_first();
			if (histext == NULL) {
				g_signal_emit_by_name(GTK_WIDGET(cl), "search_not_found");
				return 0;
			}
		} else {
			histext = gtk_entry_get_text(GTK_ENTRY(cl));
		}
		while (true) {
			string s = histext;
			string::size_type i;
			i = s.find(*cl->hist_word);
			if (i != string::npos && !(begin && i != 0)) {
				const char *tmp = gtk_entry_get_text(GTK_ENTRY(cl));
				if (!(avance && strcmp(tmp, histext) == 0)) {
					gtk_entry_set_text(GTK_ENTRY(cl), 
					g_locale_to_utf8 (histext, -1, NULL, NULL, NULL));
					g_signal_emit_by_name(GTK_WIDGET(cl), "search_letter");
					return 1;
				}
			}
			histext = cl->hist->prev_to_first();
			if (histext == NULL) {
				g_signal_emit_by_name(GTK_WIDGET(cl), "search_not_found");
				break;
			}
		}
	} else {
		g_signal_emit_by_name(GTK_WIDGET(cl), "search_letter");
	}

	return 0;
}

static int search_forward_history(GtkCompletionLine* cl, bool avance, bool begin)
{
	if (!cl->hist_word->empty()) {
		const char * histext;
		if (avance) {
			histext = cl->hist->next_to_last();
			if (histext == NULL) {
				g_signal_emit_by_name(GTK_WIDGET(cl), "search_not_found");
				return 0;
			}
		} else {
			histext = gtk_entry_get_text(GTK_ENTRY(cl));
		}
		while (true) {
			string s = histext;
			string::size_type i;
			i = s.find(*cl->hist_word);
			if (i != string::npos && !(begin && i != 0)) {
				const char *tmp = gtk_entry_get_text(GTK_ENTRY(cl));
				if (!(avance && strcmp(tmp, histext) == 0)) {
					gtk_entry_set_text(GTK_ENTRY(cl), 
					g_locale_to_utf8 (histext, -1, NULL, NULL, NULL));
					g_signal_emit_by_name(GTK_WIDGET(cl), "search_letter");
					return 1;
				}
			}
			histext = cl->hist->next_to_last();
			if (histext == NULL) {
				g_signal_emit_by_name(GTK_WIDGET(cl), "search_not_found");
				break;
			}
		}
	} else {
		g_signal_emit_by_name(GTK_WIDGET(cl), "search_letter");
	}

	return 0;
}

static int search_history(GtkCompletionLine* cl, bool avance, bool begin)
{
	switch (cl->hist_search_mode) {
	case GCL_SEARCH_REW:
	case GCL_SEARCH_BEG:
		return search_back_history(cl, avance, begin);

	case GCL_SEARCH_FWD:
		return search_forward_history(cl, avance, begin);

	default:
		return -1;
	}
}

static void search_off(GtkCompletionLine* cl)
{
	int pos = gtk_editable_get_position(GTK_EDITABLE(cl));
	cl->hist_search_mode = GCL_SEARCH_OFF;
	g_signal_emit_by_name(GTK_WIDGET(cl), "search_mode");
	cl->hist->reset_position();
}

#define STOP_PRESS \
	(g_signal_stop_emission_by_name(GTK_WIDGET(cl),   "key_press_event"))
#define MODE_BEG \
	(cl->hist_search_mode == GCL_SEARCH_BEG)
#define MODE_REW \
	(cl->hist_search_mode == GCL_SEARCH_REW)
#define MODE_FWD \
	(cl->hist_search_mode == GCL_SEARCH_FWD)
#define MODE_SRC \
	(cl->hist_search_mode != GCL_SEARCH_OFF)

static guint tab_pressed(GtkCompletionLine* cl)
{
	if (MODE_SRC)
		search_off(cl);
	complete_line(cl);
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
		if (MODE_SRC) {
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
		if (MODE_SRC) {
			search_off(cl);
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean
on_key_press(GtkCompletionLine *cl, GdkEventKey *event, gpointer data)
{
	static guint timeout_id = 0;

	if (event->type == GDK_KEY_PRESS) {
		switch (event->keyval) {
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
			STOP_PRESS;
			return TRUE;

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
			if (MODE_SRC) {
				search_off(cl);
			}
			STOP_PRESS;
			return TRUE;

		case GDK_KEY_space:
		{
			cl->first_key = 0;
			bool search = MODE_SRC;
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
			if (MODE_SRC) {
				search_off(cl);
			}
			STOP_PRESS;
			return TRUE;

		case GDK_KEY_Return:
			if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			}
			if (event->state & GDK_CONTROL_MASK) {
				g_signal_emit_by_name(GTK_WIDGET(cl), "runwithterm");
			} else {
				g_signal_emit_by_name(GTK_WIDGET(cl), "activate");
			}
			STOP_PRESS;
			return TRUE;

		case GDK_KEY_exclam:
			if (!MODE_BEG) {
				if (!MODE_SRC)
					gtk_editable_delete_selection(GTK_EDITABLE(cl));
				const char *tmp = gtk_entry_get_text(GTK_ENTRY(cl));
				if (!(*tmp == '\0' || cl->first_key))
					goto ordinary;
				cl->hist_search_mode = GCL_SEARCH_BEG;
				cl->hist_word->clear();
				g_signal_emit_by_name(GTK_WIDGET(cl), "search_mode");
				STOP_PRESS;
				return true;
			} else goto ordinary;

		case GDK_KEY_R:
		case GDK_KEY_r:
			if (event->state & GDK_CONTROL_MASK) {
				if (MODE_SRC) {
					search_back_history(cl, true, MODE_BEG);
				} else {
					cl->hist_search_mode = GCL_SEARCH_REW;
					cl->hist_word->clear();
					cl->hist->reset_position();
					g_signal_emit_by_name(GTK_WIDGET(cl), "search_mode");
				}
				STOP_PRESS;
				return TRUE;
			} else goto ordinary;

		case GDK_KEY_S:
		case GDK_KEY_s:
			if (event->state & GDK_CONTROL_MASK) {
				if (MODE_SRC) {
					search_forward_history(cl, true, MODE_BEG);
				} else {
					cl->hist_search_mode = GCL_SEARCH_FWD;
					cl->hist_word->clear();
					cl->hist->reset_position();
					g_signal_emit_by_name(GTK_WIDGET(cl), "search_mode");
				}
				STOP_PRESS;
				return TRUE;
			} else goto ordinary;

		case GDK_KEY_BackSpace:
			if (MODE_SRC) {
				if (!cl->hist_word->empty()) {
					cl->hist_word->erase(cl->hist_word->length() - 1);
					g_signal_emit_by_name(GTK_WIDGET(cl), "search_letter");
				}
				STOP_PRESS;
				return TRUE;
			}
			return FALSE;

		case GDK_KEY_Home:
		case GDK_KEY_End:
			clear_selection(cl);
			goto ordinary;

		case GDK_KEY_Escape:
			if (MODE_SRC) {
				search_off(cl);
			} else if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			} else {
				// user cancelled
				g_signal_emit_by_name(GTK_WIDGET(cl), "cancel");
			}
			STOP_PRESS;
			return TRUE;

		case GDK_KEY_G:
		case GDK_KEY_g:
			if (event->state & GDK_CONTROL_MASK) {
				search_off(cl);
				if (MODE_SRC)
					STOP_PRESS;
				return TRUE;
			} else goto ordinary;

		case GDK_KEY_E:
		case GDK_KEY_e:
			if (event->state & GDK_CONTROL_MASK) {
				search_off(cl);
				if (MODE_SRC)
					clear_selection(cl);
			}
			goto ordinary;

		ordinary:
		default:
			cl->first_key = 0;
			if (cl->win_compl != NULL) {
				gtk_widget_destroy(cl->win_compl);
				cl->win_compl = NULL;
			}
			cl->where = NULL;
			if (MODE_SRC) {
				if (event->length > 0) {
					*cl->hist_word += event->string;
					if (search_history(cl, false, MODE_BEG) <= 0)
						cl->hist_word->erase(cl->hist_word->length() - 1);
					STOP_PRESS;
					return TRUE;
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
	} //if
	return FALSE;
}

#undef STOP_PRESS
#undef MODE_BEG
#undef MODE_REW
#undef MODE_FWD
#undef MODE_SRC

// Local Variables: ***
// mode: c++ ***
// c-basic-offset: 2 ***
// End: ***
