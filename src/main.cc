/*****************************************************************************
 *  $Id: main.cc,v 1.13 2001/07/18 07:03:39 mishoo Exp $
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
on_compline_activated(GtkCompletionLine *cl, gpointer data)
{
  const char *progname = gtk_entry_get_text(GTK_ENTRY(cl));
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
    tmp += " ";
    tmp += command;
    tmp += " &";
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

static void
on_compline_unique(GtkCompletionLine *cl, GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "unique");
  gtk_widget_set_style(label, style_unique(label));
}

static void
on_compline_notunique(GtkCompletionLine *cl, GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "not unique");
  gtk_widget_set_style(label, style_notunique(label));
}

static void
on_compline_incomplete(GtkCompletionLine *cl, GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "not found");
  gtk_widget_set_style(label, style_notfound(label));
}

static gint
search_off_timeout(GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "Run program:");
  return FALSE;
}

static void
on_search_mode(GtkCompletionLine *cl, GtkWidget *label)
{
  if (cl->hist_search_mode != GCL_SEARCH_OFF) {
    gtk_widget_show(label);
    gtk_label_set_text(GTK_LABEL(label), "Search:");
  } else {
    gtk_widget_show(label);
    gtk_label_set_text(GTK_LABEL(label), "Search OFF");
    gtk_timeout_add(1000, GtkFunction(search_off_timeout), label);
  }
}

static void
on_search_letter(GtkCompletionLine *cl, GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), cl->hist_word->c_str());
}

static gint
search_fail_timeout(GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "Search:");
  gtk_widget_set_style(label, style_notunique(label));

  return FALSE;
}

static void
on_search_not_found(GtkCompletionLine *cl, GtkWidget *label)
{
  gtk_label_set_text(GTK_LABEL(label), "Not Found!");
  gtk_widget_set_style(label, style_notfound(label));
  gtk_timeout_add(1000, GtkFunction(search_fail_timeout), label);
}

int main(int argc, char **argv)
{
  GtkWidget *win;
  GtkWidget *compline;
  GtkWidget *label_search;

  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
  gtk_misc_set_alignment(GTK_MISC(label_search), 0.0, 1.0);
  gtk_misc_set_padding(GTK_MISC(label_search), 10, 0);
  gtk_box_pack_start(GTK_BOX(hhbox), label_search, TRUE, TRUE, 0);

  GtkAccelGroup *accels = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(win), accels);

  compline = gtk_completion_line_new();
  gtk_widget_set_name(compline, "Msh_Run_Compline");
  int prefs_width;
  if (!configuration.get_int("Width", prefs_width))
    prefs_width = 500;

  gtk_widget_set_usize(compline, prefs_width, -2);
  gtk_widget_add_accelerator(compline, "destroy", accels,
                             GDK_Escape, 0, (GtkAccelFlags)0);
  gtk_signal_connect(GTK_OBJECT(compline), "destroy",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(compline), "activate",
                     GTK_SIGNAL_FUNC(on_compline_activated), NULL);
  gtk_signal_connect(GTK_OBJECT(compline), "runwithterm",
                     GTK_SIGNAL_FUNC(on_compline_runwithterm), NULL);

  gtk_signal_connect(GTK_OBJECT(compline), "unique",
                     GTK_SIGNAL_FUNC(on_compline_unique), label);
  gtk_signal_connect(GTK_OBJECT(compline), "notunique",
                     GTK_SIGNAL_FUNC(on_compline_notunique), label);
  gtk_signal_connect(GTK_OBJECT(compline), "incomplete",
                     GTK_SIGNAL_FUNC(on_compline_incomplete), label);

  gtk_signal_connect(GTK_OBJECT(compline), "search_mode",
                     GTK_SIGNAL_FUNC(on_search_mode), label);
  gtk_signal_connect(GTK_OBJECT(compline), "search_not_found",
                     GTK_SIGNAL_FUNC(on_search_not_found), label);
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
