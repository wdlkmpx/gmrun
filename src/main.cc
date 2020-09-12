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

#ifdef MTRACE
#include <mcheck.h>
#endif

#include <unistd.h>
#include <errno.h>

#include "gtkcompletionline.h"
#include "config_prefs.h"

enum
{
	W_TEXT_STYLE_NORMAL,
	W_TEXT_STYLE_NOTFOUND,
	W_TEXT_STYLE_NOTUNIQUE,
	W_TEXT_STYLE_UNIQUE,
};

static void gmrun_exit (void);
GtkAllocation window_geom = { -1, -1, -1, -1 };
GtkWidget * compline;

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
static void set_info_text_color (GtkWidget *w, const char *text, int spec)
{
	char *markup = NULL;
	static const char * colors[] = {
		"black", /* W_TEXT_STYLE_NORMAL */
		"red",   /* W_TEXT_STYLE_NOTFOUND */
		"blue",  /* W_TEXT_STYLE_NOTUNIQUE */
		"green", /* W_TEXT_STYLE_UNIQUE */
	};
	markup = g_markup_printf_escaped ("<span foreground=\"%s\">%s</span>",
	                                  colors[spec], text);
	if (markup) {
		gtk_label_set_markup (GTK_LABEL (w), markup);
		g_free(markup);
	}
}

static void
run_the_command(const char * cmd, struct gigi* g)
{
#if DEBUG
	fprintf (stderr, "command: %s\n", cmd);
#endif
	GError * error = NULL;
	gboolean success;
	int argc;
	char ** argv;
	success = g_shell_parse_argv (cmd, &argc, &argv, &error);
	if (!success) {
		set_info_text_color (g->w1, error->message, W_TEXT_STYLE_NOTFOUND);
		g_error_free (error);
		add_search_off_timeout (3000, g);
		return;
	}

	success = g_spawn_async (NULL, argv, NULL,
	                         G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
	if (argv) {
		g_strfreev (argv);
	}
	if (success) {
		gmrun_exit ();
	} else {
		set_info_text_color (g->w1, error->message, W_TEXT_STYLE_NOTFOUND);
		g_error_free (error);
		add_search_off_timeout (3000, g);
	}
}

static void
on_ext_handler(GtkCompletionLine *cl, const char* ext, struct gigi* g)
{
	if (!ext) {
		search_off_timeout (g);
		return;
	}
	const char * handler = config_get_handler_for_extension (ext);
	if (handler) {
		char * tmp = g_strconcat ("Handler: ", handler, NULL);
		gtk_label_set_text (GTK_LABEL(g->w2), tmp);
		gtk_widget_show (g->w2);
		g_free (tmp);
	}
}

static void on_compline_runwithterm (GtkCompletionLine *cl, struct gigi* g)
{
	char cmd[512];
	char * term;
	char * entry_text = g_strdup (gtk_entry_get_text (GTK_ENTRY(cl)));
	g_strstrip (entry_text);

	if (*entry_text) {
		if (config_get_string_expanded ("TermExec", &term)) {
			snprintf (cmd, sizeof (cmd), "%s %s", term, entry_text);
			g_free (term);
		} else {
			snprintf (cmd, sizeof (cmd), "xterm -e %s", entry_text);
		}
	} else {
		if (config_get_string ("Terminal", &term)) {
			strncpy (cmd, term, sizeof (cmd));
		} else {
			strncpy (cmd, "xterm", sizeof (cmd));
		}
	}

	history_append (cl->hist, cmd);
	run_the_command (cmd, g);
	g_free (entry_text);
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
	if (cl->hist_search_mode == TRUE) {
		gtk_widget_show(g->w2);
		gtk_label_set_text(GTK_LABEL(g->w1), "Search:");
		gtk_label_set_text(GTK_LABEL(g->w2), cl->hist_word);
	} else {
		gtk_widget_hide(g->w2);
		gtk_label_set_text(GTK_LABEL(g->w1), "Search OFF");
		add_search_off_timeout(1000, g);
	}
}

static void
on_search_letter(GtkCompletionLine *cl, GtkWidget *label)
{
	gtk_label_set_text (GTK_LABEL(label), cl->hist_word);
}

static gint
search_fail_timeout(struct gigi *g)
{
	set_info_text_color(g->w1, "Search:", W_TEXT_STYLE_NOTUNIQUE);
	g_search_off_timeout_id = 0;
	return FALSE;
}

static void
on_search_not_found(GtkCompletionLine *cl, struct gigi *g)
{
	set_info_text_color(g->w1, "Not Found!", W_TEXT_STYLE_NOTFOUND);
	add_search_off_timeout(1000, g, GSourceFunc(search_fail_timeout));
}

static bool url_check(GtkCompletionLine *cl, struct gigi *g, char * entry_text)
{
	// <url_type> <delim>  <url>
	// http          :     //www.fsf.org
	//  <f  u  l  l     u  r  l>
	// config: URL_<url_type>
	// handler %s (format 1) = run handler with <url>
	// handler %u (format 2) = run handler with <full url>
	char * cmd;
	char * tmp, * delim, * p;
	char * url, * url_type, * full_url, * chosen_url;
	char * url_handler;
	char * config_key;

	delim = strchr (entry_text, ':');
	if (!delim || !*(delim+1)) {
		return FALSE;
	}
	tmp    = g_strdup (entry_text);
	delim  = strchr (tmp, ':');
	*delim = 0;
	url_type = tmp;       // http
	url      = delim + 1; // //www.fsf.org
	full_url = entry_text;

	config_key = g_strconcat ("URL_", url_type);
	if (config_get_string_expanded (config_key, &url_handler))
	{
		chosen_url = url;
		p = strchr (url_handler, '%');
		if (p) { // handler %s
			p++;
			if (*p == 'u') { // handler %u
				*p = 's';    // convert %u to %s (for printf)
				chosen_url = full_url;
			}
			cmd = g_strdup_printf (url_handler, chosen_url);
		} else {
			cmd = g_strconcat (url_handler, " ", url, NULL);
		}
		g_free (url_handler);
	}

	if (cmd) {
		history_append (cl->hist, cmd);
		run_the_command (cmd, g);
		g_free (cmd);
	} else {
		g_free (tmp);
		tmp = g_strconcat ("No URL handler for [", config_key, "]", NULL);
		set_info_text_color (g->w1, tmp, W_TEXT_STYLE_NOTFOUND);
		add_search_off_timeout (1000, g);
	}

	g_free (entry_text);
	g_free (config_key);
	g_free (tmp);
	return TRUE;
}

static bool ext_check (GtkCompletionLine *cl, struct gigi *g, char * entry_text)
{
	// example: file.html -> `xdg-open %s` -> `xdg-open file.html`
	char * cmd;
	char * ext = strrchr (entry_text, '.');
	char * handler_format = NULL;
	if (ext) {
		handler_format = config_get_handler_for_extension (ext);
	}

	if (handler_format) {
		if (strchr (handler_format, '%')) { // xdg-open %s
			cmd = g_strdup_printf (handler_format, entry_text);
		} else { // xdg-open
			cmd = g_strconcat (handler_format, " ", entry_text, NULL);
		}
		history_append (cl->hist, cmd);
		run_the_command (cmd, g);

		g_free (entry_text);
		g_free (cmd);
		return TRUE;
	}

	return FALSE;
}

static void on_compline_activated (GtkCompletionLine *cl, struct gigi *g)
{
	char * entry_text = g_strdup (gtk_entry_get_text (GTK_ENTRY(cl)));
	g_strstrip (entry_text);

	if (url_check(cl, g, entry_text))
		return;
	if (ext_check(cl, g, entry_text))
		return;

	char cmd[512];
	char * AlwaysInTerm = NULL;
	char ** term_progs = NULL;
	char * selected_term_prog = NULL;

	if (config_get_string ("AlwaysInTerm", &AlwaysInTerm))
	{
		term_progs = g_strsplit (AlwaysInTerm, " ", 0);
		int i;
		for (i = 0; term_progs[i]; i++) {
			if (strcmp (term_progs[i], entry_text) == 0) {
				selected_term_prog = g_strdup (term_progs[i]);
				break;
			}
		}
		g_strfreev (term_progs);
	}

	if (selected_term_prog) {
		char * TermExec;
		config_get_string_expanded ("TermExec", &TermExec);
		snprintf (cmd, sizeof (cmd), "%s %s", TermExec, selected_term_prog);
		g_free (selected_term_prog);
		g_free (TermExec);
	} else {
		strncpy (cmd, entry_text, sizeof (cmd));
	}
	g_free (entry_text);

	history_append (cl->hist, cmd);
	run_the_command (cmd, g);
}

// =============================================================

static void gmrun_activate(void)
{
	GtkWidget *dialog, * main_vbox;

	GtkWidget *label_search;
	struct gigi g;

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
	g_signal_connect(G_OBJECT(dialog), "destroy",
						G_CALLBACK(gmrun_exit), NULL);

	GtkWidget *hhbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (GTK_BOX (main_vbox), hhbox, FALSE, FALSE, 0);

	GtkWidget *label = gtk_label_new("Run program:");
	gtk_box_pack_start (GTK_BOX(hhbox), label, FALSE, FALSE, 10);
	gtkcompat_widget_set_halign_left (GTK_WIDGET (label));

	label_search = gtk_label_new("");
	gtk_box_pack_end (GTK_BOX (hhbox), label_search, TRUE, TRUE, 10);
	gtkcompat_widget_set_halign_right (GTK_WIDGET (label_search));

	compline = gtk_completion_line_new();
	gtk_widget_set_name (compline, "gmrun_compline");
	gtk_box_pack_start (GTK_BOX (main_vbox), compline, TRUE, TRUE, 0);

	// don't show files starting with "." by default
	if (config_get_int ("ShowDotFiles", &(GTK_COMPLETION_LINE(compline)->show_dot_files))) {
		GTK_COMPLETION_LINE(compline)->show_dot_files = 0;
	}
	int tmp;
	if (config_get_int ("TabTimeout", &tmp)) {
		((GtkCompletionLine*)compline)->tabtimeout = tmp;
	}

	g.w1 = label;
	g.w2 = label_search;

	g_signal_connect(G_OBJECT(compline), "cancel",
						G_CALLBACK(gmrun_exit), NULL);
	g_signal_connect(G_OBJECT(compline), "activate",
						G_CALLBACK(on_compline_activated), &g);
	g_signal_connect(G_OBJECT(compline), "runwithterm",
						G_CALLBACK(on_compline_runwithterm), &g);

	g_signal_connect(G_OBJECT(compline), "unique",
						G_CALLBACK(on_compline_unique), &g);
	g_signal_connect(G_OBJECT(compline), "notunique",
						G_CALLBACK(on_compline_notunique), &g);
	g_signal_connect(G_OBJECT(compline), "incomplete",
						G_CALLBACK(on_compline_incomplete), &g);

	g_signal_connect(G_OBJECT(compline), "search_mode",
						G_CALLBACK(on_search_mode), &g);
	g_signal_connect(G_OBJECT(compline), "search_not_found",
						G_CALLBACK(on_search_not_found), &g);
	g_signal_connect(G_OBJECT(compline), "search_letter",
						G_CALLBACK(on_search_letter), label_search);

	g_signal_connect(G_OBJECT(compline), "ext_handler",
						G_CALLBACK(on_ext_handler), &g);

	int shows_last_history_item;
	if (!config_get_int ("ShowLast", &shows_last_history_item)) {
		shows_last_history_item = 0;
	}
	if (shows_last_history_item) {
		gtk_completion_line_last_history_item(GTK_COMPLETION_LINE(compline));
	}

	// geometry: window position
	if (window_geom.x > -1 || window_geom.y > -1) {
		gtk_window_move (GTK_WINDOW (dialog), window_geom.x, window_geom.y);
	} else {
		/* default: centered */
		gtk_window_set_position (GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
	}

	// geometry: window size
	if (window_geom.height > -1 || window_geom.width > -1) {
		gtk_window_set_default_size (GTK_WINDOW (dialog), window_geom.width,
		                             window_geom.height);
	} else {
		/* default width = 450 */
		gtk_window_set_default_size (GTK_WINDOW (dialog), 450, -1);
	}

	// window icon
	GError * error = NULL;
	GtkIconTheme * theme = gtk_icon_theme_get_default ();
	GdkPixbuf * icon = gtk_icon_theme_load_icon (theme, "gmrun", 48, GTK_ICON_LOOKUP_USE_BUILTIN, &error);
	if (error) {
		g_object_set (dialog, "icon-name", "gtk-execute", NULL);
		g_error_free (error);
	} else {
		gtk_window_set_icon (GTK_WINDOW (dialog), icon);
		g_object_unref (icon);
	}

	gtk_widget_show_all (dialog);

	gtk_window_set_focus(GTK_WINDOW(dialog), compline);
}

// =============================================================

static void parse_command_line (int argc, char ** argv)
{
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

	if (!geometry_str)
	{
		// --geometry was not specified, see config file
		char * geomstr;
		if (config_get_string ("Geometry", &geomstr)) {
			geometry_str = g_strdup (geomstr);
		}
	}

	if (geometry_str)
	{
		// --geometry WxH+X+Y
		// width x height + posX + posY
		int width, height, posX, posY;
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
			window_geom.x = posX;
			window_geom.y = posY;
		}

		Hstr = strchr (geometry_str, 'x');
		if (Hstr) { // WxH
			*Hstr = 0;
			Hstr++; // H
			Wstr = geometry_str;
			width  = strtoll (Wstr, NULL, 0);
			height = strtoll (Hstr, NULL, 0);
			///fprintf (stderr, "w: %" G_GINT64_FORMAT "\nh: %" G_GINT64_FORMAT "\n", width, height);
			window_geom.width = width;
			window_geom.height = height;
		}

		g_free (geometry_str);
	}
}


// =============================================================
//                           MAIN

void gmrun_exit(void)
{
	gtk_widget_destroy (compline);
	config_destroy ();
	gtk_main_quit ();
}

int main(int argc, char **argv)
{

#ifdef MTRACE
	mtrace();
#endif

	gtk_init(&argc, &argv);

	config_init ();
	parse_command_line (argc, argv);
	gmrun_activate ();

	gtk_main();

	return (0);
}

