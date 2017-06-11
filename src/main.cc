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
#include <popt.h>

using namespace std;

#ifdef MTRACE
#include <mcheck.h>
#endif

#include <unistd.h>
#include <errno.h>

#include "gtkcompletionline.h"
#include "prefs.h"

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
    gtk_timeout_remove(g_search_off_timeout_id);
    g_search_off_timeout_id = 0;
  }
}

static void add_search_off_timeout(guint32 timeout, struct gigi *g, GtkFunction func = 0)
{
  remove_search_off_timeout();
  if (!func)
    func = GtkFunction(search_off_timeout);
  g_search_off_timeout_id = gtk_timeout_add(timeout, func, g);
}

/// END: TIMEOUT MANAGEMENT

GtkStyle* style_normal(GtkWidget *w)
{
  static GtkStyle *style;

  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x0000, 0x0000, 0x0000};
    gtk_style_ref(style);
  }
  return style;
}

GtkStyle* style_notfound(GtkWidget *w)
{
  static GtkStyle *style;

  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0xFFFF, 0x0000, 0x0000};
    gtk_style_ref(style);
  }
  return style;
}

GtkStyle* style_notunique(GtkWidget *w)
{
  static GtkStyle *style;

  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x0000, 0x0000, 0xFFFF};
    gtk_style_ref(style);
  }
  return style;
}

GtkStyle* style_unique(GtkWidget *w)
{
  static GtkStyle *style;

  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x0000, 0xFFFF, 0x0000};
    gtk_style_ref(style);
  }
  return style;
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
    gtk_label_set_text(GTK_LABEL(g->w1), error.c_str());
    gtk_widget_set_style(g->w1, style_notfound(g->w1));
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
    // gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
  } else {
    search_off_timeout(g);
  }
}

static void
on_compline_runwithterm(GtkCompletionLine *cl, struct gigi* g)
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

static gint
search_off_timeout(struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "Run program:");
  gtk_widget_set_style(g->w1, style_normal(g->w1));
  gtk_widget_hide(g->w2);
  return FALSE;
}

static void
on_compline_unique(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "unique");
  gtk_widget_set_style(g->w1, style_unique(g->w1));
  add_search_off_timeout(1000, g);
}

static void
on_compline_notunique(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "not unique");
  gtk_widget_set_style(g->w1, style_notunique(g->w1));
  add_search_off_timeout(1000, g);
}

static void
on_compline_incomplete(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "not found");
  gtk_widget_set_style(g->w1, style_notfound(g->w1));
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
  gtk_label_set_text(GTK_LABEL(g->w1), "Search:");
  gtk_widget_set_style(g->w2, style_notunique(g->w2));

  return FALSE;
}

static void
on_search_not_found(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "Not Found!");
  gtk_widget_set_style(g->w2, style_notfound(g->w2));
  add_search_off_timeout(1000, g, GtkFunction(search_fail_timeout));
}

static bool
url_check(GtkCompletionLine *cl, struct gigi *g)
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
      gtk_label_set_text(GTK_LABEL(g->w1),
                         (string("No URL handler for [") +
                          url_type + "]").c_str());
      gtk_widget_set_style(g->w1, style_notfound(g->w1));
      add_search_off_timeout(1000, g);
      return true;
    }
  }

  return false;
}

static bool
ext_check(GtkCompletionLine *cl, struct gigi *g)
{
  vector<string> words;
  get_words(cl, words);
  vector<string>::const_iterator
    i     = words.begin(),
    i_end = words.end();

  while (i != i_end) {
    const string& w = quote_string(*i++);
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
    // FIXME: for now we check only one entry
    break;
  }

  return false;
}

static void
on_compline_activated(GtkCompletionLine *cl, struct gigi *g)
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

/**
 Check if screen contain ponter and return coords
 Taked from Xfce: libxfcegui4
 */
static gboolean
screen_contains_pointer (GdkScreen *screen,
                         int       *x,
                         int       *y)
{
    GdkWindow    *root_window;
    Window        root, child;
    Bool          retval;
    int           rootx, rooty;
    int           winx, winy;
    unsigned int  xmask;

    root_window = gdk_screen_get_root_window (screen);

    retval = XQueryPointer (GDK_SCREEN_XDISPLAY (screen),
                            GDK_DRAWABLE_XID (root_window),
                            &root, &child, &rootx, &rooty,
                            &winx, &winy, &xmask);

    if (x)
        *x = retval ? rootx : -1;
    if (y)
        *y = retval ? rooty : -1;

    return retval;
}

/**
 Found monitor that contain mouse pointer
 Taked from Xfce: libxfcegui4
 */
static GdkScreen*
gmrun_gdk_display_locate_monitor_with_pointer (GdkDisplay *display,
                                               gint       *monitor_return)
{
    int n_screens, i;

    if (display == NULL)
        display = gdk_display_get_default ();

    n_screens = gdk_display_get_n_screens (display);
    for (i = 0; i < n_screens; i++)
    {
        GdkScreen  *screen;
        int         x, y;

        screen = gdk_display_get_screen (display, i);

        if (screen_contains_pointer (screen, &x, &y))
        {
            if (monitor_return)
                *monitor_return = gdk_screen_get_monitor_at_point (screen, x, y);

            return screen;
        }
    }

    if (monitor_return)
        *monitor_return = 0;

    return NULL;
}

/**
 Center window on given monitor
  Taked from Xfce: libxfcegui4
 */
static void
//gmrun_gtk_window_center_on_monitor (GtkWindow *window,
gmrun_gtk_window_place_on_monitor (GtkWindow *window,
                                   GdkScreen *screen,
                                   gint       monitor,
                                   int        request_x = -1,
                                   int        request_y = -1)

{
    GtkRequisition requisition;
    GdkRectangle   geometry;
    GdkScreen     *widget_screen;
    gint           x, y;

    gdk_screen_get_monitor_geometry (screen, monitor, &geometry);

    /*
     * Getting a size request requires the widget
     * to be associated with a screen, because font
     * information may be needed (Olivier).
     */
    widget_screen = gtk_widget_get_screen (GTK_WIDGET (window));
    if (screen != widget_screen)
    {
        gtk_window_set_screen (GTK_WINDOW (window), screen);
    }
    /*
     * We need to be realized, otherwise we may get
     * some odd side effects (Olivier).
     */
    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (window)))
    {
        gtk_widget_realize (GTK_WIDGET (window));
    }
    /*
     * Yes, I know -1 is useless here (Olivier).
     */
    requisition.width = requisition.height = -1;
    gtk_widget_size_request (GTK_WIDGET (window), &requisition);

    if( request_x < 0 )
    {
        x = geometry.x + (geometry.width - requisition.width) / 2;
    }
    else
    {
        x = geometry.x + request_x;
    }

    if( request_y < 0 )
    {
        y = geometry.y + (geometry.height - requisition.height - 200) / 2;
    }
    else
    {
        y = geometry.y + request_y;
    }

    gtk_window_move (window, x, y);
}

static void
gmrun_gtk_window_place_at(GtkWindow *window,
                        int request_x,
                        int request_y,
                        int centered_width = 0,
                        int centered_height = 0,
                        int on_active = 0)
{
    GdkScreen *screen = NULL;
    gint       monitor = 0;

    if(on_active != 0)
    {
        screen = gmrun_gdk_display_locate_monitor_with_pointer(NULL, &monitor);
    }

    if( screen == NULL )
    {
        screen = gdk_screen_get_default();
        monitor = 0;
    }

    if(centered_width != 0)
    {
        request_x = -1;
    }

    if(centered_height != 0)
    {
        request_y = -1;
    }

    gmrun_gtk_window_place_on_monitor(window,
                                      screen,
                                      monitor,
                                      request_x,
                                      request_y);
}

int main(int argc, char **argv)
{
  GtkWidget *dialog;
  GtkWidget *compline;
  GtkWidget *label_search;
  struct gigi g;

#ifdef MTRACE
  mtrace();
#endif

  gtk_init(&argc, &argv);

  dialog = gtk_dialog_new();
  gtk_widget_realize(dialog);
  gdk_window_set_decorations(dialog->window, GDK_DECOR_BORDER);
  gtk_widget_set_name(dialog, "Msh_Run_Window");
  gtk_window_set_title(GTK_WINDOW(dialog), "Execute program feat. completion");
  gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, TRUE);
  // gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  GtkWidget *hhbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hhbox);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hhbox, FALSE, FALSE, 0);

  GtkWidget *label = gtk_label_new("Run program:");
  gtk_widget_show(label);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_misc_set_padding(GTK_MISC(label), 10, 0);
  gtk_box_pack_start(GTK_BOX(hhbox), label, FALSE, FALSE, 0);

  label_search = gtk_label_new("");
  gtk_widget_show(label_search);
  gtk_misc_set_alignment(GTK_MISC(label_search), 1.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label_search), 10, 0);
  gtk_box_pack_start(GTK_BOX(hhbox), label_search, TRUE, TRUE, 0);

  compline = gtk_completion_line_new();
  gtk_widget_set_name(compline, "Msh_Run_Compline");
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

  gtk_widget_set_usize(compline, prefs_width, -2);
  gtk_signal_connect(GTK_OBJECT(compline), "cancel",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(compline), "activate",
                     GTK_SIGNAL_FUNC(on_compline_activated), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "runwithterm",
                     GTK_SIGNAL_FUNC(on_compline_runwithterm), &g);

  gtk_signal_connect(GTK_OBJECT(compline), "unique",
                     GTK_SIGNAL_FUNC(on_compline_unique), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "notunique",
                     GTK_SIGNAL_FUNC(on_compline_notunique), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "incomplete",
                     GTK_SIGNAL_FUNC(on_compline_incomplete), &g);

  gtk_signal_connect(GTK_OBJECT(compline), "search_mode",
                     GTK_SIGNAL_FUNC(on_search_mode), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "search_not_found",
                     GTK_SIGNAL_FUNC(on_search_not_found), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "search_letter",
                     GTK_SIGNAL_FUNC(on_search_letter), label_search);

  gtk_signal_connect(GTK_OBJECT(compline), "ext_handler",
                     GTK_SIGNAL_FUNC(on_ext_handler), &g);
  gtk_widget_show(compline);

  int shows_last_history_item;
  if (!configuration.get_int("ShowLast", shows_last_history_item)) {
    shows_last_history_item = 0;
  }
  if (shows_last_history_item) {
    gtk_completion_line_last_history_item(GTK_COMPLETION_LINE(compline));
  }

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), compline, TRUE, TRUE, 0);

  int prefs_top = 80;
  int prefs_left = 100;
  int prefs_centred_by_width = 0;
  int prefs_centred_by_height = 0;
  int prefs_use_active_monitor = 0;
  configuration.get_int("Top", prefs_top);
  configuration.get_int("Left", prefs_left);
  configuration.get_int("CenteredByWidth", prefs_centred_by_width);
  configuration.get_int("CenteredByHeight", prefs_centred_by_height);
  configuration.get_int("UseActiveMonitor", prefs_use_active_monitor);

  // parse commandline options
  gboolean geo_parsed;
  char geo_option[30] = "";
  char *geoptr;
  poptContext context;
  int option;

  geoptr = geo_option;

  struct poptOption options[] = {
    { "geometry", 'g', POPT_ARG_STRING | POPT_ARGFLAG_ONEDASH,
      &geoptr, 0, "This option specifies the initial "
      "size and location of the window.", NULL },
    POPT_AUTOHELP
    { NULL, '\0', 0, NULL, 0 }
  };

  context = poptGetContext("popt1", argc, (const char**) argv, options, 0);
  option = poptGetNextOpt (context);

  if (strcmp (geoptr, ""))
  {
    geo_parsed = gtk_window_parse_geometry (GTK_WINDOW (dialog),
                        geoptr);
  }
  else
  {
      gmrun_gtk_window_place_at(GTK_WINDOW(dialog),
                                prefs_left,
                                prefs_top,
                                prefs_centred_by_width,
                                prefs_centred_by_height,
                                prefs_use_active_monitor);
  }

  gtk_widget_show(dialog);

  gtk_window_set_focus(GTK_WINDOW(dialog), compline);

  gtk_main();
}

// Local Variables: ***
// mode: c++ ***
// c-basic-offset: 2 ***
// End: ***
