/*
 * Copyright 2020 Mihai Bazon
 * 
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided that
 * the above copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 */

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

#define CMD_LENGTH 1024

enum
{
   W_TEXT_STYLE_NORMAL,
   W_TEXT_STYLE_NOTFOUND,
   W_TEXT_STYLE_NOTUNIQUE,
   W_TEXT_STYLE_UNIQUE,
};

GtkApplication * gmrun_app;

char * gmrun_text = NULL;
static void gmrun_exit (void);
GtkAllocation window_geom = { -1, -1, -1, -1 };
/* widgets that are used in several functions */
GtkWidget * compline;
GtkWidget * wlabel;
GtkWidget * wlabel_search;

/* preferences */
int USE_GLIB_XDG = 0;
int SHELL_RUN = 1;

/// BEGIN: TIMEOUT MANAGEMENT

static gboolean search_off_timeout ();
static guint g_search_off_timeout_id = 0;

static void remove_search_off_timeout (void)
{
   if (g_search_off_timeout_id) {
      g_source_remove(g_search_off_timeout_id);
      g_search_off_timeout_id = 0;
   }
}

static void add_search_off_timeout (guint32 timeout, GSourceFunc func)
{
   remove_search_off_timeout();
   if (!func)
      func = (GSourceFunc) search_off_timeout;
   g_search_off_timeout_id = g_timeout_add (timeout, func, NULL);
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


static void run_the_command (char * cmd)
{
#if DEBUG
   fprintf (stderr, "command: %s\n", cmd);
#endif
 if (SHELL_RUN)
 {
   // need to add extra &
   if (strlen (cmd) < (CMD_LENGTH-10)) {
      strcat (cmd, " &"); /* safe to use in this case */
   }
   int ret = system (cmd);
   if (ret != -1) {
      gmrun_exit ();
   } else {
      char errmsg[256];
      snprintf (errmsg, sizeof(errmsg)-1, "ERROR: %s", strerror (errno));
      set_info_text_color (wlabel, errmsg, W_TEXT_STYLE_NOTFOUND);
      add_search_off_timeout (3000, NULL);
   }
 }
 else // glib - more conservative approach and robust error reporting
 {
   GError * error = NULL;
   gboolean success;
   int argc;
   char ** argv;
   success = g_shell_parse_argv (cmd, &argc, &argv, &error);
   if (!success) {
      set_info_text_color (wlabel, error->message, W_TEXT_STYLE_NOTFOUND);
      g_error_free (error);
      add_search_off_timeout (3000, NULL);
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
      set_info_text_color (wlabel, error->message, W_TEXT_STYLE_NOTFOUND);
      g_error_free (error);
      add_search_off_timeout (3000, NULL);
   }
 }
}


static void
on_ext_handler (GtkCompletionLine *cl, const char * filename)
{
 if (USE_GLIB_XDG) // GLib XDG handling (freedesktop specification)
 {
   gchar * content_type, * mime_type, * msg;
   const gchar * handler;
   GAppInfo * app_info;

   if (filename) {
      content_type = g_content_type_guess (filename, NULL, 0, NULL);
      if (content_type) {
         mime_type = g_content_type_get_mime_type (content_type);
         g_free (content_type);
         app_info = g_app_info_get_default_for_type (mime_type, FALSE);
         g_free (mime_type);
         if (app_info) {
            handler = g_app_info_get_commandline (app_info);
            msg = g_strconcat("Handler: ", handler, NULL);
            gtk_label_set_text (GTK_LABEL (wlabel_search), msg);
            gtk_widget_show (wlabel_search);

            g_object_unref(app_info);
            g_free(msg);
            return;
         }
      }
   }
   search_off_timeout();
 }
 else // custom EXT handlers
 {
   const char * ext = strrchr (filename, '.');
   if (!ext) {
      search_off_timeout ();
      return;
   }
   const char * handler = config_get_handler_for_extension (ext);
   if (handler) {
      char * tmp = g_strconcat ("Handler: ", handler, NULL);
      gtk_label_set_text (GTK_LABEL (wlabel_search), tmp);
      gtk_widget_show (wlabel_search);
      g_free (tmp);
   }
 }
}


static void on_compline_runwithterm (GtkCompletionLine *cl)
{
   char cmd[CMD_LENGTH];
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
         strncpy (cmd, term, sizeof (cmd) - 1);
      } else {
         strncpy (cmd, "xterm", sizeof (cmd) - 1);
      }
   }

   history_append (cl->hist, cmd);
   run_the_command (cmd);
   g_free (entry_text);
}

static gboolean search_off_timeout ()
{
   set_info_text_color (wlabel, "Run program:", W_TEXT_STYLE_NORMAL);
   gtk_widget_hide (wlabel_search);
   g_search_off_timeout_id = 0;
   return G_SOURCE_REMOVE;
}

static void
on_compline_unique (GtkCompletionLine *cl)
{
   set_info_text_color (wlabel, "unique", W_TEXT_STYLE_UNIQUE);
   add_search_off_timeout (1000, NULL);
}

static void
on_compline_notunique (GtkCompletionLine *cl)
{
   set_info_text_color (wlabel, "not unique", W_TEXT_STYLE_NOTUNIQUE);
   add_search_off_timeout (1000, NULL);
}

static void
on_compline_incomplete (GtkCompletionLine *cl)
{
   set_info_text_color (wlabel, "not found", W_TEXT_STYLE_NOTFOUND);
   add_search_off_timeout (1000, NULL);
}

static void
on_search_mode (GtkCompletionLine *cl)
{
   if (cl->hist_search_mode == TRUE) {
      gtk_widget_show (wlabel_search);
      gtk_label_set_text (GTK_LABEL (wlabel), "Search:");
      gtk_label_set_text (GTK_LABEL (wlabel_search), cl->hist_word);
   } else {
      gtk_widget_hide (wlabel_search);
      gtk_label_set_text (GTK_LABEL (wlabel), "Search OFF");
      add_search_off_timeout (1000, NULL);
   }
}

static void
on_search_letter(GtkCompletionLine *cl, GtkWidget *label)
{
   gtk_label_set_text (GTK_LABEL(label), cl->hist_word);
}

static gboolean search_fail_timeout (gpointer user_data)
{
   set_info_text_color (wlabel, "Search:", W_TEXT_STYLE_NOTUNIQUE);
   g_search_off_timeout_id = 0;
   return G_SOURCE_REMOVE;
}

static void
on_search_not_found(GtkCompletionLine *cl)
{
   set_info_text_color (wlabel, "Not Found!", W_TEXT_STYLE_NOTFOUND);
   add_search_off_timeout (1000, (GSourceFunc) search_fail_timeout);
}


// =============================================================

static void xdg_app_run_command (GAppInfo *app, const gchar *args)
{
   // get
   char * cmd, * exe;
   GRegex * regex;

   regex = g_regex_new (".%[fFuUdDnNickvm]", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
   exe = g_regex_replace_literal (regex, // Remove xdg desktop files fields from app
                                  g_app_info_get_commandline (app),
                                  -1, 0, "", G_REGEX_MATCH_NOTEMPTY, NULL);
   cmd = g_strconcat (exe, " ", args, NULL);
   run_the_command (cmd); /* Launch the command */
   g_regex_unref (regex);
   g_free (exe);
   g_free (cmd);
}

/* Handler for URLs  */
static gboolean url_check (GtkCompletionLine *cl, char * entry_text)
{
 if (USE_GLIB_XDG) // GLib XDG handling (freedesktop specification)
 {
   char * delim;
   const char * url, * protocol;
   GAppInfo * app;
   delim = strchr (entry_text, ':');
   if (!delim || !*(delim+1)) {
      return FALSE;
   }
   protocol = entry_text;
   url = delim + 1;
   *delim = 0;

   if (url[0] == '/' && url[1] == '/')
   {
      app = g_app_info_get_default_for_uri_scheme (protocol);
      if (app) { // found known uri handler for protocol
         *delim = ':';
         xdg_app_run_command (app, entry_text);
         history_append (cl->hist, entry_text);
         g_object_unref (app);
      } else {
         char *tmp = g_strconcat ("No URL handler for [", protocol, "]", NULL);
         set_info_text_color (wlabel, tmp, W_TEXT_STYLE_NOTFOUND);
         add_search_off_timeout (1000, NULL);
         g_free (tmp);
      }
      return TRUE;
   }
   *delim = ':';
   return FALSE;

 }
 else //-------- custom URL handlers
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
   cmd = tmp = delim = p = url_handler = config_key = NULL;
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

   config_key = g_strconcat ("URL_", url_type, NULL);
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
      history_append (cl->hist, entry_text);
      run_the_command (cmd);
      g_free (cmd);
   } else {
      g_free (tmp);
      tmp = g_strconcat ("No URL handler for [", config_key, "]", NULL);
      set_info_text_color (wlabel, tmp, W_TEXT_STYLE_NOTFOUND);
      add_search_off_timeout (1000, NULL);
   }
   g_free (config_key);
   g_free (tmp);
   return TRUE;
 }
}


static char * escape_spaces (char * entry_text)
{  // run file with glib: replace " " with "\ "
   GRegex * regex;
   char * quoted;
   if (!strstr (entry_text, "\\ ")) {
      regex = g_regex_new (" ", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY, NULL);
      quoted = g_regex_replace_literal (regex, entry_text, -1, 0, "\\ ", G_REGEX_MATCH_NOTEMPTY, NULL);
      g_regex_unref (regex);
   } else {
      quoted = strdup (entry_text); // already scaped, just duplicate text
   }
   return (quoted);
}

/* Handler for extensions */
static gboolean ext_check (GtkCompletionLine *cl, char * entry_text)
{
 if (USE_GLIB_XDG) // GLib XDG handling (freedesktop specification)
 {
   char *quoted, *content_type, *mime_type;
   GAppInfo *app_info;
   gboolean sure;
   quoted = escape_spaces (entry_text);
   /* File is executable: launch it (fail silently if file isn't really executable) */
   if (g_file_test (quoted, G_FILE_TEST_IS_EXECUTABLE)) {
      run_the_command (quoted);
      history_append (cl->hist, entry_text);
      g_free (quoted);
      return TRUE;
   }
   /* Check mime type through extension */
   if (quoted[0] == '/' && strchr (quoted, '.')) {
      content_type = g_content_type_guess (quoted, NULL, 0, &sure);
      if (content_type) {
         mime_type = g_content_type_get_mime_type (content_type);
         g_free (content_type);
         app_info = g_app_info_get_default_for_type (mime_type, FALSE);
         g_free (mime_type);
         if (app_info) { // found mime
            xdg_app_run_command (app_info, quoted);
            history_append (cl->hist, entry_text);
            g_free (quoted);
            g_object_unref(app_info);
            return TRUE;
         }
      }
   }
   g_free (quoted);
   return FALSE;

 }
 else //-------- custom EXTension handlers
 {
   // example: file.html | xdg-open '%s' -> xdg-open 'file.html'
   char * cmd, * quoted;
   char * ext = strrchr (entry_text, '.');
   char * handler_format = NULL;
   if (ext) {
      handler_format = config_get_handler_for_extension (ext);
   }
   if (handler_format) {
      quoted = g_strcompress (entry_text); /* unescape chars */
      if (strstr (handler_format, "%s")) {
         cmd = g_strdup_printf (handler_format, quoted);
      }
      else { // xdg-open
         cmd = g_strconcat (handler_format, " '", quoted, "'", NULL);
      }
      history_append (cl->hist, entry_text);
      run_the_command (cmd);
      g_free (cmd);
      g_free (quoted);
      return TRUE;
   }

   return FALSE;
 }
}

// =============================================================

static void on_compline_activated (GtkCompletionLine *cl)
{
   char * entry_text = g_strdup (gtk_entry_get_text (GTK_ENTRY(cl)));
   g_strstrip (entry_text);

   if (url_check (cl, entry_text) == TRUE
       || ext_check (cl, entry_text) == TRUE)  {
      g_free (entry_text);
      return;
   }

   char cmd[CMD_LENGTH];
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
      strncpy (cmd, entry_text, sizeof (cmd) - 1);
   }
   g_free (entry_text);

   history_append (cl->hist, cmd);
   run_the_command (cmd);
}


// =============================================================


static void gmrun_activate(void)
{
   GtkWidget *dialog, * main_vbox;
   GtkWidget *label_search;

   GtkWidget * window = gtk_application_window_new (gmrun_app);
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
   wlabel = label;

   label_search = gtk_label_new("");
   gtk_box_pack_start (GTK_BOX (hhbox), label_search, FALSE, TRUE, 0);
   wlabel_search = label_search;

   compline = gtk_completion_line_new();
   gtk_widget_set_name (compline, "gmrun_compline");
   gtk_box_pack_start (GTK_BOX (main_vbox), compline, TRUE, TRUE, 0);

   if (!config_get_int ("SHELL_RUN", &SHELL_RUN)) {
      SHELL_RUN = 1;
   }

   // don't show files starting with "." by default
   if (!config_get_int ("ShowDotFiles", &(GTK_COMPLETION_LINE(compline)->show_dot_files))) {
      GTK_COMPLETION_LINE(compline)->show_dot_files = 0;
   }
   int tmp;
   if (!config_get_int ("TabTimeout", &tmp)) {
      ((GtkCompletionLine*)compline)->tabtimeout = tmp;
   }
   if (!config_get_int ("USE_GLIB_XDG", &USE_GLIB_XDG)) {
      USE_GLIB_XDG = 0;
   }

   g_signal_connect(G_OBJECT(compline), "cancel",
                    G_CALLBACK(gmrun_exit), NULL);
   g_signal_connect(G_OBJECT(compline), "activate",
                    G_CALLBACK (on_compline_activated), NULL);
   g_signal_connect(G_OBJECT(compline), "runwithterm",
                    G_CALLBACK (on_compline_runwithterm), NULL);

   g_signal_connect(G_OBJECT(compline), "unique",
                    G_CALLBACK (on_compline_unique), NULL);
   g_signal_connect(G_OBJECT(compline), "notunique",
                    G_CALLBACK (on_compline_notunique), NULL);
   g_signal_connect(G_OBJECT(compline), "incomplete",
                    G_CALLBACK (on_compline_incomplete), NULL);

   g_signal_connect(G_OBJECT(compline), "search_mode",
                    G_CALLBACK (on_search_mode), NULL);
   g_signal_connect(G_OBJECT(compline), "search_not_found",
                    G_CALLBACK (on_search_not_found), NULL);
   g_signal_connect(G_OBJECT(compline), "search_letter",
                    G_CALLBACK(on_search_letter), label_search);

   g_signal_connect(G_OBJECT(compline), "ext_handler",
                    G_CALLBACK (on_ext_handler), NULL);

   int shows_last_history_item;
   if (!config_get_int ("ShowLast", &shows_last_history_item)) {
      shows_last_history_item = 0;
   }
   if (gmrun_text) {
      gtk_entry_set_text (GTK_ENTRY(compline), gmrun_text);
   } else if (shows_last_history_item) {
      gtk_completion_line_last_history_item (GTK_COMPLETION_LINE(compline));
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
   static char *geometry_str = NULL;
   GError *error = NULL;
   GOptionContext *context = NULL;
   static GOptionEntry entries[] =
   {
      { "geometry", 'g', 0, G_OPTION_ARG_STRING, &geometry_str, "This option specifies the initial size and location of the window.", NULL, },
      { NULL },
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
   if (argc >= 2) {
      gmrun_text = argv[1];
   }
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
   g_application_quit (G_APPLICATION (gmrun_app));
}

int main(int argc, char **argv)
{

#ifdef MTRACE
   mtrace();
#endif
   int status = 0;

   config_init ();
   parse_command_line (argc, argv);

#if GTK_CHECK_VERSION(3, 4, 0)
   // Handling cmd line args with GApplication is a nightmare
   // follow this: https://developer.gnome.org/gtkmm-tutorial/stable/sec-multi-item-containers.html.en#boxes-command-line-options
   argc = 1; /* hide args from GApplication */
   gmrun_app = gtk_application_new ("org.gtk.gmrun", G_APPLICATION_NON_UNIQUE);
   g_signal_connect (gmrun_app, "activate", gmrun_activate, NULL);
   status = g_application_run (G_APPLICATION (gmrun_app), argc, argv);
   g_object_unref (gmrun_app);
#else
   gtk_init (&argc, &argv);
   gmrun_activate ();
   gtk_main ();
#endif

#ifdef MTRACE
   muntrace();
#endif

   return (status);
}

