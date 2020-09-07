/*****************************************************************************
 *  $Id: main.cc,v 1.26 2003/11/16 10:55:07 andreas99 Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/

#include <X11/Xlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>

using namespace std;

#ifdef MTRACE
#include <mcheck.h>
#endif

#include <unistd.h>
#include <errno.h>

#include "gtkcompletionline.h"
#include "prefs.h"
#include "main.h"

// defined in gtkcompletionline.cc
int get_words(GtkCompletionLine *object, vector<string>& words);
string quote_string(const string& str);

struct gigi
{
	GtkWidget *w1;
	GtkWidget *w2;
};

/// BEGIN: TIMEOUT MANAGEMENT

static gint search_off_timeout(struct gigi *g);

static guint g_search_off_timeout_id = 0;

static void remove_search_off_timeout()
{
	if (g_search_off_timeout_id) {
		g_source_remove(g_search_off_timeout_id);
		g_search_off_timeout_id = 0;
	}
}

static void add_search_off_timeout(guint32 timeout, struct gigi *g, GSourceFunc func = 0)
{
	remove_search_off_timeout();
	if (!func)
		func = GSourceFunc(search_off_timeout);
	g_search_off_timeout_id = g_timeout_add(timeout, func, g);
}

/// END: TIMEOUT MANAGEMENT

// https://unix.stackexchange.com/questions/457584/gtk3-change-text-color-in-a-label-raspberry-pi
static void set_info_text_color(GtkWidget *w, const char *text, int spec) {
	char *markup = NULL;
	switch (spec) {
	case W_TEXT_STYLE_NORMAL: // black
		markup = g_markup_printf_escaped ("<span foreground=\"black\">%s</span>", text);
		break;
	case W_TEXT_STYLE_NOTFOUND: // red
		markup = g_markup_printf_escaped ("<span foreground=\"red\">%s</span>", text);
		break;
	case W_TEXT_STYLE_NOTUNIQUE: // blue
		markup = g_markup_printf_escaped ("<span foreground=\"blue\">%s</span>", text);
		break;
	case W_TEXT_STYLE_UNIQUE: //green
		markup = g_markup_printf_escaped ("<span foreground=\"green\">%s</span>", text);
		break;
	default:
		markup = g_markup_printf_escaped ("<span foreground=\"black\">%s</span>", text);
	} //switch
	if (markup) {
		gtk_label_set_markup (GTK_LABEL (w), markup);
		g_free(markup);
	}
}

static void
run_the_command(const std::string& command, struct gigi* g)
{
	string cmd(command);
	cmd += '&';
	int ret = system(cmd.c_str());
#ifdef DEBUG
	cerr << "System exit code: " << ret << endl;
#endif
	if (ret != -1)
		gtk_main_quit();
	else {
		string error("ERROR: ");
		error += strerror(errno);
#ifdef DEBUG
		cerr << error << endl;
#endif
		set_info_text_color(g->w1, error.c_str(), W_TEXT_STYLE_NOTFOUND);
		add_search_off_timeout(2000, g);
	}
}

static void
on_ext_handler(GtkCompletionLine *cl, const char* ext, struct gigi* g)
{
	string cmd;
	if (ext && configuration.get_ext_handler(ext, cmd)) {
		string str("Handler: ");
		size_t pos = cmd.find_first_of(" \t");
		if (pos == string::npos)
			str += cmd;
		else
			str += cmd.substr(0, pos);
		gtk_label_set_text(GTK_LABEL(g->w2), str.c_str());
		gtk_widget_show(g->w2);
		// gtk_timeout_add(1000, GSourceFunc(search_off_timeout), g);
	} else {
		search_off_timeout(g);
	}
}

static void on_compline_runwithterm(GtkCompletionLine *cl, struct gigi* g)
{
	string command(g_locale_from_utf8 (gtk_entry_get_text(GTK_ENTRY(cl)),
						-1,
						NULL,
						NULL,
						NULL));
	string tmp;
	string term;

	string::size_type i;
	i = command.find_first_not_of(" \t");

	if (i != string::npos) {
		if (!configuration.get_string("TermExec", term)) {
			term = "xterm -e";
		}
		tmp = term;
		tmp += " ";
		tmp += command;
	} else {
		if (!configuration.get_string("Terminal", term)) {
			tmp = "xterm";
		} else {
			tmp = term;
		}
	}

#ifdef DEBUG
	cerr << tmp << endl;
#endif

	cl->hist->append(command.c_str());
	cl->hist->sync_the_file();
	run_the_command(tmp.c_str(), g);
}

static gint search_off_timeout(struct gigi *g)
{
	set_info_text_color(g->w1, "Run program:", W_TEXT_STYLE_NORMAL);
	gtk_widget_hide(g->w2);
	g_search_off_timeout_id = 0;
	return FALSE;
}

static void
on_compline_unique(GtkCompletionLine *cl, struct gigi *g)
{
	set_info_text_color(g->w1, "unique", W_TEXT_STYLE_UNIQUE);
	add_search_off_timeout(1000, g);
}

static void
on_compline_notunique(GtkCompletionLine *cl, struct gigi *g)
{
	set_info_text_color(g->w1, "not unique", W_TEXT_STYLE_NOTUNIQUE);
	add_search_off_timeout(1000, g);
}

static void
on_compline_incomplete(GtkCompletionLine *cl, struct gigi *g)
{
	set_info_text_color(g->w1, "not found", W_TEXT_STYLE_NOTFOUND);
	add_search_off_timeout(1000, g);
}

static void
on_search_mode(GtkCompletionLine *cl, struct gigi *g)
{
	if (cl->hist_search_mode != GCL_SEARCH_OFF) {
		gtk_widget_show(g->w2);
		gtk_label_set_text(GTK_LABEL(g->w1), "Search:");
		gtk_label_set_text(GTK_LABEL(g->w2), cl->hist_word->c_str());
	} else {
		gtk_widget_hide(g->w2);
		gtk_label_set_text(GTK_LABEL(g->w1), "Search OFF");
		add_search_off_timeout(1000, g);
	}
}

static void
on_search_letter(GtkCompletionLine *cl, GtkWidget *label)
{
	gtk_label_set_text(GTK_LABEL(label), cl->hist_word->c_str());
}

static gint
search_fail_timeout(struct gigi *g)
{
	set_info_text_color(g->w1, "Search:", W_TEXT_STYLE_NOTUNIQUE);
	return FALSE;
}

static void
on_search_not_found(GtkCompletionLine *cl, struct gigi *g)
{
	set_info_text_color(g->w1, "Not Found!", W_TEXT_STYLE_NOTFOUND);
	add_search_off_timeout(1000, g, GSourceFunc(search_fail_timeout));
}

static bool url_check(GtkCompletionLine *cl, struct gigi *g)
{
	string text(g_locale_from_utf8 (gtk_entry_get_text(GTK_ENTRY(cl)),
					-1,
					NULL,
					NULL,
					NULL));

	string::size_type i;
	string::size_type sp;

	sp = text.find_first_not_of(" \t");
	if (sp == string::npos) return true;
	text = text.substr(sp);

	sp = text.find_first_of(" \t");
	i = text.find(":");

	if (i != string::npos && i < sp) {
		// URL entered...
		string url(text.substr(i + 1));
		string url_type(text.substr(0, i));
		string url_handler;

		if (configuration.get_string(string("URL_") + url_type, url_handler)) {
			string::size_type j = 0;

			do {
				j = url_handler.find("%s", j);
				if (j != string::npos) {
					url_handler.replace(j, 2, url);
				}
			} while (j != string::npos);

			j = 0;
			do {
				j = url_handler.find("%u", j);
				if (j != string::npos) {
					url_handler.replace(j, 2, text);
				}
			} while (j != string::npos);

			cl->hist->append(text.c_str());
			cl->hist->sync_the_file();
			run_the_command(url_handler.c_str(), g);
			return true;
		} else {
			set_info_text_color(g->w1,
							(string("No URL handler for [") + url_type + "]").c_str(),
							W_TEXT_STYLE_NOTFOUND);
			add_search_off_timeout(1000, g);
			return true;
		}
	}

	return false;
}

static bool ext_check(GtkCompletionLine *cl, struct gigi *g)
{
	vector<string> words;
	get_words(cl, words);
	vector<string>::const_iterator
	i     = words.begin(),
	i_end = words.end();


	GRegex *regex = g_regex_new (" ", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
	while (i != i_end) {
		gchar *quoted = g_regex_replace_literal (
				regex, (*i++).c_str(), -1, 0, "\\ ", G_REGEX_MATCH_NOTEMPTY, NULL);
		const string w = quoted;
		if (w[0] == '/') {
			// absolute path, check for extension
			size_t pos = w.rfind('.');
			if (pos != string::npos) {
				// we have extension
				string ext = w.substr(pos + 1);
				string ext_handler;
				if (configuration.get_ext_handler(ext, ext_handler)) {
					// we have the handler
					pos = ext_handler.find("%s");
					if (pos != string::npos)
						ext_handler.replace(pos, 2, w);
					cl->hist->append(w.c_str());
					cl->hist->sync_the_file();
					run_the_command(ext_handler.c_str(), g);
					return true;
				}
			}
		}
		g_free(quoted);
		// FIXME: for now we check only one entry
		break;
	}
	g_regex_unref(regex);

	return false;
}

static void on_compline_activated(GtkCompletionLine *cl, struct gigi *g)
{
	if (url_check(cl, g))
		return;
	if (ext_check(cl, g))
		return;

	string command = g_locale_from_utf8 (gtk_entry_get_text(GTK_ENTRY(cl)),
						-1,
						NULL,
						NULL,
						NULL);

	string::size_type i;
	i = command.find_first_not_of(" \t");

	if (i != string::npos) {
		string::size_type j = command.find_first_of(" \t", i);
		string progname = command.substr(i, j - i);
		list<string> term_progs;
		if (configuration.get_string_list("AlwaysInTerm", term_progs)) {
#ifdef DEBUG
			cerr << "---" << std::endl;
			std::copy(term_progs.begin(), term_progs.end(),
					std::ostream_iterator<string>(cerr, "\n"));
			cerr << "---" << std::endl;
#endif
			list<string>::const_iterator w =
			std::find(term_progs.begin(), term_progs.end(), progname);
			if (w != term_progs.end()) {
				on_compline_runwithterm(cl, g);
				return;
			}
		}
		cl->hist->append(command.c_str());
		cl->hist->sync_the_file();
		run_the_command(command, g);
	}
}


int main(int argc, char **argv)
{
	GtkWidget *dialog, * main_vbox;
	GtkWidget *compline;
	GtkWidget *label_search;
	struct gigi g;

#ifdef MTRACE
	mtrace();
#endif

	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	dialog = gtk_dialog_new();
	gtk_window_set_transient_for( (GtkWindow*)dialog, (GtkWindow*)window );
	gtk_widget_realize(dialog);
	main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	// this removes the title bar..
	GdkWindow *gwin = gtk_widget_get_window (GTK_WIDGET(dialog));
	gdk_window_set_decorations (gwin, GDK_DECOR_BORDER);

	gtk_widget_set_name (GTK_WIDGET (dialog), "gmrun");
	gtk_window_set_title (GTK_WINDOW(window), "A simple launcher with completion");

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);
	g_signal_connect(GTK_WIDGET(dialog), "destroy",
						G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget *hhbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (GTK_BOX (main_vbox), hhbox, FALSE, FALSE, 0);

	GtkWidget *label = gtk_label_new("Run program:");
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(hhbox), label, FALSE, FALSE, 10);

	label_search = gtk_label_new("");
	gtk_widget_set_halign (label, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (hhbox), label_search, TRUE, TRUE, 10);

	compline = gtk_completion_line_new();
	gtk_widget_set_name (compline, "gmrun_compline");
	gtk_box_pack_start (GTK_BOX (main_vbox), compline, TRUE, TRUE, 0);

	int prefs_width;
	if (!configuration.get_int("Width", prefs_width))
		prefs_width = 500;

	// don't show files starting with "." by default
	if (!configuration.get_int("ShowDotFiles", GTK_COMPLETION_LINE(compline)->show_dot_files))
		GTK_COMPLETION_LINE(compline)->show_dot_files = 0;
	{
		int tmp;
		if (configuration.get_int("TabTimeout", tmp))
		((GtkCompletionLine*)compline)->tabtimeout = tmp;
	}

	g.w1 = label;
	g.w2 = label_search;

	gtk_widget_set_size_request(compline, prefs_width, -1);
	g_signal_connect(GTK_WIDGET(compline), "cancel",
						G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(GTK_WIDGET(compline), "activate",
						G_CALLBACK(on_compline_activated), &g);
	g_signal_connect(GTK_WIDGET(compline), "runwithterm",
						G_CALLBACK(on_compline_runwithterm), &g);

	g_signal_connect(GTK_WIDGET(compline), "unique",
						G_CALLBACK(on_compline_unique), &g);
	g_signal_connect(GTK_WIDGET(compline), "notunique",
						G_CALLBACK(on_compline_notunique), &g);
	g_signal_connect(GTK_WIDGET(compline), "incomplete",
						G_CALLBACK(on_compline_incomplete), &g);

	g_signal_connect(GTK_WIDGET(compline), "search_mode",
						G_CALLBACK(on_search_mode), &g);
	g_signal_connect(GTK_WIDGET(compline), "search_not_found",
						G_CALLBACK(on_search_not_found), &g);
	g_signal_connect(GTK_WIDGET(compline), "search_letter",
						G_CALLBACK(on_search_letter), label_search);

	g_signal_connect(GTK_WIDGET(compline), "ext_handler",
						G_CALLBACK(on_ext_handler), &g);

	int shows_last_history_item;
	if (!configuration.get_int("ShowLast", shows_last_history_item)) {
		shows_last_history_item = 0;
	}
	if (shows_last_history_item) {
		gtk_completion_line_last_history_item(GTK_COMPLETION_LINE(compline));
	}

	int prefs_top = 80;
	int prefs_left = 100;
	configuration.get_int("Top", prefs_top);
	configuration.get_int("Left", prefs_left);

	// --geometry / parse commandline options
	char *geometry_str = NULL;
	GError *error = NULL;
	GOptionContext *context = NULL;
	static GOptionEntry entries[] =
	{
		{ "geometry", 'g', 0, G_OPTION_ARG_STRING, &geometry_str, "This option specifies the initial size and location of the window.", NULL },
		{ NULL }
	};

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error))
	{
		g_print ("option parsing failed: %s\n", error->message);
		if (context)      g_option_context_free (context);
		if (error)        g_error_free (error);
		if (geometry_str) g_free (geometry_str);
		exit (1);
	}
	if (context) g_option_context_free (context);
	if (error)   g_error_free (error);
	// --

	if (!geometry_str) {
		gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
	} else {
		// --geometry WxH+X+Y
		// width x height + posX + posY
		gint64 width, height, posX, posY;
		char *Wstr, *Hstr, *Xstr, *Ystr;
		Wstr = Hstr = Xstr = Ystr = NULL;

		Xstr = strchr (geometry_str, '+');
		if (Xstr) { // +posX+posY
			*Xstr = 0;
			Xstr++; // posX+posY
			Ystr = strchr (Xstr, '+');
			if (Ystr) { // +posY
				*Ystr = 0;
				Ystr++; // posY
			}
		}
		if (Xstr && Ystr && *Xstr && *Ystr) {
			posX = strtoll (Xstr, NULL, 0);
			posY = strtoll (Ystr, NULL, 0);
			///fprintf (stderr, "x: %" G_GINT64_FORMAT "\ny: %" G_GINT64_FORMAT "\n", posX, posY);
			gtk_window_move (GTK_WINDOW (dialog), posX, posY);
		}

		Hstr = strchr (geometry_str, 'x');
		if (Hstr) { // WxH
			*Hstr = 0;
			Hstr++; // H
			Wstr = geometry_str;
			width  = strtoll (Wstr, NULL, 0);
			height = strtoll (Hstr, NULL, 0);
			///fprintf (stderr, "w: %" G_GINT64_FORMAT "\nh: %" G_GINT64_FORMAT "\n", width, height);
			gtk_window_set_default_size (GTK_WINDOW (dialog), width, height);
		}

		g_free (geometry_str);
	}

	gtk_widget_show_all (dialog);

	gtk_window_set_focus(GTK_WINDOW(dialog), compline);

	gtk_main();
}

