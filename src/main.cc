/*****************************************************************************
 *  $Id: main.cc,v 1.3 2001/05/03 07:44:14 mishoo Exp $
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
#include "history.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string>
using namespace std;

GtkStyle* style_notfound(GtkWidget *w)
{
  static GtkStyle *style;
    
  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x8000, 0x0000, 0x0000};
    gtk_style_ref(style);
  }
  return style;
}

GtkStyle* style_notunique(GtkWidget *w)
{
  static GtkStyle *style;
    
  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x0000, 0x0000, 0x8000};
    gtk_style_ref(style);
  }
  return style;
}

GtkStyle* style_unique(GtkWidget *w)
{
  static GtkStyle *style;
    
  if (!style) {
    style = gtk_style_copy(gtk_widget_get_style(w));
    style->fg[GTK_STATE_NORMAL] = (GdkColor){0, 0x0000, 0x8000, 0x0000};
    gtk_style_ref(style);
  }
  return style;
}

static void
on_compline_activated(GtkEntry *entry, gpointer data)
{
  const char *progname = gtk_entry_get_text(entry);
  string tmp = progname;
  tmp += " &";
    
  system(tmp.c_str());
  history.append(progname);
  gtk_main_quit();
}

static void
on_compline_runwithterm(GtkCompletionLine *cl, gpointer data)
{
  const char *progname = gtk_entry_get_text(GTK_ENTRY(cl));
  string tmp("xterm -e ");
  tmp += progname;
  tmp += " &";
    
  system(tmp.c_str());
  history.append(progname);
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

static void
on_compline_uparrow(GtkCompletionLine *cl, GtkWidget *label)
{
  history.set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
  gtk_entry_set_text(GTK_ENTRY(cl), history.prev());
}

static void
on_compline_dnarrow(GtkCompletionLine *cl, GtkWidget *label)
{
  history.set_default(gtk_entry_get_text(GTK_ENTRY(cl)));
  gtk_entry_set_text(GTK_ENTRY(cl), history.next());
}

int main(int argc, char **argv)
{
  GtkWidget *win;
  GtkWidget *compline;
    
  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), "Execute program feat. completion");
  gtk_window_set_policy(GTK_WINDOW(win), FALSE, FALSE, TRUE);
  gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(win), 4);
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
                     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  GtkWidget *hbox = gtk_vbox_new(FALSE, 2);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(win), hbox);

  GtkWidget *label = gtk_label_new("Run program:");
  gtk_widget_show(label);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
  gtk_misc_set_padding(GTK_MISC(label), 10, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  GtkAccelGroup *accels = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(win), accels);
    
  compline = gtk_completion_line_new();
  gtk_widget_set_usize(compline, 500, -2);
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
  gtk_signal_connect(GTK_OBJECT(compline), "uparrow",
                     GTK_SIGNAL_FUNC(on_compline_uparrow), label);
  gtk_signal_connect(GTK_OBJECT(compline), "dnarrow",
                     GTK_SIGNAL_FUNC(on_compline_dnarrow), label);
  gtk_widget_show(compline);
    
  gtk_box_pack_start(GTK_BOX(hbox), compline, TRUE, TRUE, 0);

  gtk_widget_show(win);
  gtk_window_set_focus(GTK_WINDOW(win), compline);
    
  gtk_main();
}
