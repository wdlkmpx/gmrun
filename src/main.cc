/*****************************************************************************
 *  $Id: main.cc,v 1.16 2001/07/27 08:01:02 mishoo Exp $
 *  Copyright (C) 2000, Mishoo
 *  Author: Mihai Bazon                  Email: mishoo@fenrir.infoiasi.ro
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


#include "gtkcompletionline.h"

#include "prefs.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string>
#include <iostream>
using namespace std;

struct gigi
{
  GtkWidget *w1;
  GtkWidget *w2;
};

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

int
IsStringBlank(const char *str)
{
  int i=-1;
  while(++i<strlen(str)) {
	if( str[i] != ' ' )
      return 0;
  }
  return 1;
}

static void
on_compline_runwithterm(GtkCompletionLine *cl, gpointer data)
{
  string command(gtk_entry_get_text(GTK_ENTRY(cl)));
  string tmp;
  string term;

  string::size_type i;
  i = command.find_first_not_of(" \t");

  if (i != string::npos) {
    if (!configuration.get_string("TermExec", term)) {
      term = "xterm -e";
    }
    tmp = term;
    tmp += " \"";
    tmp += command;
    tmp += "\" &";
  } else {
    if (!configuration.get_string("Terminal", term)) {
      term = "xterm";
    }
    tmp = term + " &";
  }

#ifdef DEBUG
  cerr << tmp << endl;
#endif

  system(tmp.c_str());
  cl->hist->append(command.c_str());
  delete cl->hist;
  delete cl->hist_word;
  gtk_main_quit();
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
  gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
}

static void
on_compline_notunique(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "not unique");
  gtk_widget_set_style(g->w1, style_notunique(g->w1));
  gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
}

static void
on_compline_incomplete(GtkCompletionLine *cl, struct gigi *g)
{
  gtk_label_set_text(GTK_LABEL(g->w1), "not found");
  gtk_widget_set_style(g->w1, style_notfound(g->w1));
  gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
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
    gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
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
  gtk_timeout_add(1000, GtkFunction(search_fail_timeout), g);
}

static bool
url_check(GtkCompletionLine *cl, struct gigi *g)
{
  string text(gtk_entry_get_text(GTK_ENTRY(cl)));
  string::size_type i;

  i = text.find(":");

  if (i != string::npos) {
    // URL entered...
    string url(text.substr(i + 1));
    string url_type(text.substr(0, i));
    string url_handler;

    if (configuration.get_string(string("URL_") + url_type, url_handler)) {
      string::size_type j;

      j = url_handler.find("%s");
      if (j != string::npos) {
        url_handler.replace(j, 2, string("\"") + url + "\"");
      }
      j = url_handler.find("%u");
      if (j != string::npos) {
        url_handler.replace(j, 2, string("\"") + text + "\"");
      }
      url_handler += " &";
      system(url_handler.c_str());
      cl->hist->append(text.c_str());
      delete cl->hist;
      delete cl->hist_word;
      gtk_main_quit();
      return true;
    } else {
      gtk_label_set_text(GTK_LABEL(g->w1),
                         (string("No URL handler for [") +
                          url_type + "]").c_str());
      gtk_widget_set_style(g->w1, style_notfound(g->w1));
      gtk_timeout_add(1000, GtkFunction(search_off_timeout), g);
      return true;
    }
  }

  return false;
}

static void
on_compline_activated(GtkCompletionLine *cl, struct gigi *g)
{
  const char *progname = gtk_entry_get_text(GTK_ENTRY(cl));
  if (url_check(cl, g))
    return;

  if(!IsStringBlank(progname)) {
	string tmp = progname;
	tmp += " &";

	system(tmp.c_str());
	cl->hist->append(progname);
    delete cl->hist;
    delete cl->hist_word;
  }
  gtk_main_quit();
}

int main(int argc, char **argv)
{
  GtkWidget *win;
  GtkWidget *compline;
  GtkWidget *label_search;
  struct gigi g;

  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize(win);
  gdk_window_set_decorations(win->window, GDK_DECOR_BORDER);
  gtk_widget_set_name(win, "Msh_Run_Window");
  gtk_window_set_title(GTK_WINDOW(win), "Execute program feat. completion");
  gtk_window_set_policy(GTK_WINDOW(win), FALSE, FALSE, TRUE);
  // gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(win), 4);
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  GtkWidget *hbox = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(win), hbox);

  GtkWidget *hhbox = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hhbox);
  gtk_box_pack_start(GTK_BOX(hbox), hhbox, FALSE, FALSE, 0);

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

  GtkAccelGroup *accels = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(win), accels);

  compline = gtk_completion_line_new();
  gtk_widget_set_name(compline, "Msh_Run_Compline");
  int prefs_width;
  if (!configuration.get_int("Width", prefs_width))
    prefs_width = 500;

  g.w1 = label;
  g.w2 = label_search;

  gtk_widget_set_usize(compline, prefs_width, -2);
  gtk_widget_add_accelerator(compline, "destroy", accels,
                             GDK_Escape, 0, (GtkAccelFlags)0);
  gtk_signal_connect(GTK_OBJECT(compline), "destroy",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(compline), "activate",
                     GTK_SIGNAL_FUNC(on_compline_activated), &g);
  gtk_signal_connect(GTK_OBJECT(compline), "runwithterm",
                     GTK_SIGNAL_FUNC(on_compline_runwithterm), NULL);

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
  gtk_widget_show(compline);

  gtk_box_pack_start(GTK_BOX(hbox), compline, TRUE, TRUE, 0);

  int prefs_top = 80;
  int prefs_left = 100;
  configuration.get_int("Top", prefs_top);
  configuration.get_int("Left", prefs_left);
  gtk_widget_set_uposition(win, prefs_left, prefs_top);
  gtk_widget_show(win);

  gtk_window_set_focus(GTK_WINDOW(win), compline);

  gtk_main();
}
