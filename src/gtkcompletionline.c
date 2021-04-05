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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config_prefs.h"
#include "gtkcompletionline.h"

#define HISTORY_FILE "gmrun_history"

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

static guint gtk_completion_line_signals[LAST_SIGNAL];

static gchar ** path_gc   = NULL; /* string list (gchar *) containing each directory in PATH */
static gchar * prefix     = NULL;
static int g_show_dot_files;

/* callbacks */
static gboolean on_key_press (GtkCompletionLine *cl, GdkEventKey *event, gpointer data);
static gboolean on_scroll (GtkCompletionLine *cl, GdkEventScroll *event, gpointer data);

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

   char * history_file;
#ifdef FOLLOW_XDG_SPEC
   history_file = g_build_filename (g_get_user_data_dir(), HISTORY_FILE, NULL);
#else
   history_file = g_build_filename (g_get_home_dir(), "." HISTORY_FILE, NULL);
#endif

   int HIST_MAX_SIZE;
   if (!config_get_int ("History", &HIST_MAX_SIZE))
      HIST_MAX_SIZE = 20;

   self->hist = history_new (history_file, HIST_MAX_SIZE);
   g_free (history_file);
   // hacks for prev/next will be applied
   history_unset_current (self->hist);
}

static void gtk_completion_line_dispose (GObject *object)
{
   GtkCompletionLine * self = GTK_COMPLETION_LINE (object);
   // GTK3: Pango-CRITICAL **: pango_layout_get_cursor_pos: assertion 'index >= 0 && index <= layout->length' failed
   // -- for some reason there's an error when the object is destroyed
   // -- The GtkCompletionLine 'cancel' signal makes gmrun destroy the object and exit
   // -- The current fix is to set an empty text
   gtk_entry_set_text (GTK_ENTRY (self), "");
   // --
   if (path_gc) {
      g_strfreev (path_gc);
      path_gc = NULL;
   }
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

/* Get one word of a string: separator is space, but only unescaped spaces */
static const gchar *get_token (const gchar *str, char *out_buf, int out_buf_len)
{
   gboolean escaped = FALSE;
   *out_buf = 0;
   int x = 0;
   while (*str != '\0')
   {
      if (escaped) {
         escaped = FALSE;
         out_buf[x++] = *str;
      } else if (*str == '\\') {
         escaped = TRUE;
      } else if (isspace(*str)) {
         while (isspace(*str)) str++;
         break;
      } else {
         out_buf[x++] = *str;
      }
      if (x >= out_buf_len) {
         break;
      }
      str++;
   }
   out_buf[x] = 0;
   return str;
}

/* get words before current edit position */
int get_words (GtkCompletionLine * object, GList ** words)
{
#ifdef DEBUG
   printf ("  get_words\n");
#endif
   const gchar * content = gtk_entry_get_text (GTK_ENTRY(object));
   const gchar * i = content;
   int pos = gtk_editable_get_position (GTK_EDITABLE(object));
   int n_w = 0;
   char tmp[2048] = "";

   while (*i != '\0')
   {
      i = get_token (i, tmp, 2000);
      *words = g_list_append (*words, g_strdup(tmp));
      if (*i && (i - content < pos) && (i != content))
         n_w++;
   }

   if (!*words) { // must add empty string otherwise a segfault awaits
      *words = g_list_append (*words, strdup (""));
   }

   return n_w;
}

/* Replace words in the entry fields, return position of char after first 'pos' words */
int set_words (GtkCompletionLine *object, GList *words, int pos)
{
#ifdef DEBUG
   printf ("  set_words\n");
#endif
   GList * igl;
   char * word;
   char * tmp;
   int tmp_len = 0;

   // determine buffer length
   for (igl = words;  igl;  igl = igl->next) {
      word = (char*) (igl->data);
      tmp_len += strlen (word) + 5;
      for (tmp = word;  *tmp;  tmp++)
         if (*tmp == ' ')
            tmp_len += 5;
   }
   tmp = (char*) g_malloc0 (sizeof(char) * tmp_len);

   if (pos == -1)
      pos = g_list_length (words) - 1;
   int cur = 0;
   int i = 0;

   while (words)
   {
      // replace ' ' with '\ ' [escape]
      word = (char *)(words->data);
      while (*word) {
         if (*word == ' ') {
            tmp[i++] = '\\';
            tmp[i++] = ' ';
         } else {
            tmp[i++] = *word;
         }
         word++;
      }

      // add space if not the last word
      if (words != g_list_last (words)) {
         tmp[i++] = ' ';
      }
      if (!pos && !cur) {
         cur = strlen (tmp); /* cur: length of string after inserting pos words */
      } else {
         --pos;
      }
      words = words->next;
   }
   tmp[i] = 0;

   if (g_list_length(words) == 1) {
      g_signal_emit_by_name (G_OBJECT(object), "ext_handler", NULL);
   }

   gtk_entry_set_text (GTK_ENTRY(object), tmp);
   gtk_editable_set_position (GTK_EDITABLE(object), cur);
   g_free (tmp);
   return cur;
}

/* Filter callback for scandir */
static int select_executables_only(const struct dirent* dent)
{
   int len = strlen(dent->d_name);
   int lenp = prefix ? strlen (prefix) : 0;

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

   if (strncmp(dent->d_name, prefix, lenp) == 0)
      return 1;

   return 0;
}

/* Iterates though PATH and list all executables */
static GList * generate_execs_list (char * pfix)
{
   // generate_path_list
   if (!path_gc) {
      char *path_cstr = (char*) getenv("PATH");
      path_gc = g_strsplit (path_cstr, ":", -1);
   }

   GList * execs_gc = NULL;
   if (prefix) g_free (prefix);
   prefix = g_strdup (pfix);

   gchar ** path_gc_i = path_gc;
   while (*path_gc_i)
   {
      struct dirent **eps;
      int n = scandir (*path_gc_i, &eps, select_executables_only, NULL);
      int j;
      if (n >= 0) {
         for (j = 0; j < n; j++) {
            execs_gc = g_list_prepend (execs_gc, g_strdup (eps[j]->d_name));
            free (eps[j]);
         }
         free (eps);
      }
      path_gc_i++;
   }
   if (prefix) {
      g_free (prefix);
      prefix = NULL;
   }
   return (execs_gc);
}

/* list all subdirs in what, return if ok or not */
static GList * generate_dirlist (const char * path)
{
#ifdef DEBUG
   printf ("generate_dirlist\n");
#endif
   char * str = strdup (path);
   char * filename = strrchr (str, '/');
   GList * dirlist_gc = NULL;

   char * p = str;
   int slashes = 0;
   while (*p) {
      if (*p == '/') slashes++;
      p++;
   }

   char * dir;
   if (slashes == 1) {
      dir = "/";
   } else {
      dir = str;
   }

   *filename = '\0';
   filename++;
   if (prefix) g_free (prefix);
   prefix = g_strdup(filename);

   struct dirent **eps;
   char * file;
   int n, j, len;

   n = scandir (dir, &eps, select_executables_only, NULL);
   if (n >= 0) {
      for (j = 0; j < n; j++)
      {
         file = g_strconcat (str, "/", eps[j]->d_name, "/", NULL);
         len = strlen (file);
         file[len-1] = 0;
         struct stat filestatus;
         stat (file, &filestatus);
         if (S_ISDIR (filestatus.st_mode)) {
            file[len-1] = '/';
         }
         dirlist_gc = g_list_prepend (dirlist_gc, file);
         free(eps[j]);
      }
      free(eps);
   }

   if (prefix) {
      g_free (prefix);
      prefix = NULL;
   }
   free (str);
   return (dirlist_gc);
}

/* Expand tilde */
static int parse_tilda (GtkCompletionLine *object)
{
   const gchar *text = gtk_entry_get_text(GTK_ENTRY(object));
   const gchar *match = g_strstr_len(text, -1, "~");
   if (match) {
      gint cur = match - text;
      if ((cur > 0) && (text[cur - 1] != ' '))
         return 0;
      if ((guint)cur < strlen(text) - 1 && text[cur + 1] != '/') {
         // FIXME: Parse another user's home
      } else {
         gtk_editable_insert_text(GTK_EDITABLE(object),
                                  g_get_home_dir(), strlen(g_get_home_dir()), &cur);
         gtk_editable_delete_text(GTK_EDITABLE(object), cur, cur + 1);
      }
   }

   return 0;
}

static void complete_from_list (GtkCompletionLine *object, char *cword) /* can be NULL */
{
#ifdef DEBUG
   printf ("\ncomplete_from_list\n");
#endif
   if (on_cursor_changed_handler) {
      g_signal_handler_block (G_OBJECT(object->tree_compl), on_cursor_changed_handler);
   }

   parse_tilda(object);
   GList *words = NULL, *word_i;
   int pos = get_words (object, &words);
   word_i = g_list_nth (words, pos);
   char * word = NULL;

   /* Completion list is opened */
   if (object->win_compl != NULL) {
      /* word will point to a dynamycally allocated string */
      gtk_tree_model_get (object->sort_list_compl, &(object->list_compl_it), 0, &word, -1);

      GtkTreePath *path = gtk_tree_model_get_path(object->sort_list_compl, &(object->list_compl_it));
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(object->tree_compl), path, NULL, FALSE);
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(object->tree_compl), path, NULL, TRUE, 0.5, 0.5);
      gtk_tree_path_free(path);
   } else {
      word = cword ? strdup (cword) : NULL;
   }

   if (word) {
      g_free (word_i->data);
      word_i->data = word;
   }
   int current_pos = set_words (object, words, pos);
   gtk_editable_select_region (GTK_EDITABLE(object), object->pos_in_text, current_pos);

   g_list_free_full (words, g_free);

   if (on_cursor_changed_handler) {
      g_signal_handler_unblock (G_OBJECT(object->tree_compl), on_cursor_changed_handler);
   }
}

static void on_cursor_changed(GtkTreeView *tree, gpointer data)
{
#ifdef DEBUG
   printf ("on_cusor_changed\n");
#endif
   GtkCompletionLine *object = GTK_COMPLETION_LINE(data);

   GtkTreeSelection *selection;
   selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(object->tree_compl));
   gtk_tree_selection_get_selected (selection, &(object->sort_list_compl), &(object->list_compl_it));

   complete_from_list (object, NULL);
}

static void clear_selection (GtkCompletionLine* cl)
{
   int pos = gtk_editable_get_position (GTK_EDITABLE (cl));
   gtk_editable_select_region (GTK_EDITABLE(cl), pos, pos);
}


// called by tab_pressed() only if completion window doesn't exist
static void complete_line (GtkCompletionLine *object)
{
#ifdef DEBUG
   printf ("complete_line\n");
#endif
   parse_tilda(object);

   GList * WordList = NULL, * witem = NULL;
   GList * FileList = NULL;
   int pos = get_words (object, &WordList);
   witem = g_list_nth (WordList, pos);
   char * word = (gchar *)(witem->data);

   g_show_dot_files = object->show_dot_files;

   /* populate FileList */
   if (word[0] != '/') { /* exec list */
      FileList = generate_execs_list (word);
   } else { /* dirlist */
      FileList = generate_dirlist (word);
   }

   guint num_items = FileList ? g_list_length (FileList) : 0;

   if (num_items == 1) { // only 1 item
      complete_from_list (object, (char*)(FileList->data));
      g_signal_emit_by_name(G_OBJECT(object), "unique");
      clear_selection (object);
      g_list_free_full (WordList, g_free);
      g_list_free_full (FileList, g_free);
      return;
   } else if (num_items == 0) {
      g_signal_emit_by_name(G_OBJECT(object), "incomplete");
      g_list_free_full (WordList, g_free);
      g_list_free_full (FileList, g_free);
      return;
   }

   /*** num_items > 1 ***/
   g_signal_emit_by_name (G_OBJECT(object), "notunique");

   object->win_compl = gtk_window_new (GTK_WINDOW_POPUP);
   gtk_widget_set_name (object->win_compl, "gmrun_completion_window");

   GtkWidget * main_win = gtk_widget_get_toplevel (GTK_WIDGET (object));
   gtk_window_set_transient_for (GTK_WINDOW (object->win_compl), GTK_WINDOW (main_win));

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

   object->list_compl = gtk_list_store_new (1, G_TYPE_STRING);
   object->sort_list_compl = GTK_TREE_MODEL (object->list_compl);
   object->tree_compl = gtk_tree_view_new_with_model (object->sort_list_compl);
   g_object_unref (object->list_compl);

   /* fill ListStore before sorting */
   GtkTreeIter iter;
   GList *p = FileList;
   while (p) {
      gtk_list_store_append (object->list_compl, &iter); /* modifies iter */
      gtk_list_store_set (object->list_compl, &iter,
                          0, (char*) p->data, -1);
      p = g_list_next (p);
   }

   /* sort ListStore, column 0 */
   GtkTreeSortable * sorted = GTK_TREE_SORTABLE (object->list_compl);
   gtk_tree_sortable_set_sort_column_id (sorted, 0, GTK_SORT_ASCENDING);

   GtkTreeViewColumn *col;
   GtkCellRenderer *renderer;
   col = gtk_tree_view_column_new ();
   renderer = gtk_cell_renderer_text_new ();
   gtk_tree_view_column_pack_start (col, renderer, TRUE);
   gtk_tree_view_column_add_attribute (col, renderer, "text", 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW (object->tree_compl), col);

   GtkTreeSelection *selection;
   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object->tree_compl));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
   gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (object->tree_compl), FALSE);

   on_cursor_changed_handler = g_signal_connect (GTK_TREE_VIEW (object->tree_compl),
                                                 "cursor-changed",
                                                 G_CALLBACK (on_cursor_changed), object);
   g_signal_handler_block (G_OBJECT (object->tree_compl),
                           on_cursor_changed_handler);

   GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_OUT);
   gtk_widget_show (scroll);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   gtk_container_set_border_width (GTK_CONTAINER (object->tree_compl), 2);
   gtk_container_add (GTK_CONTAINER (scroll), object->tree_compl);
   gtk_container_add (GTK_CONTAINER (object->win_compl), scroll);

   GdkWindow *top = gtk_widget_get_parent_window (GTK_WIDGET (object));
   int x, y;
   gdk_window_get_position(top, &x, &y);
   GtkAllocation al;
   gtk_widget_get_allocation( GTK_WIDGET(object), &al );
   x += al.x;
   y += al.y + al.height;

   // gtk_widget_popup(object->win_compl, x, y);
   gtk_window_move (GTK_WINDOW (object->win_compl), x, y);
   gtk_widget_show_all (object->win_compl);

   gtk_tree_view_columns_autosize (GTK_TREE_VIEW (object->tree_compl));
   gtk_widget_get_allocation (object->tree_compl, &al);
   gtk_widget_set_size_request (scroll, al.width + 40, 150);

   gtk_tree_model_get_iter_first (object->sort_list_compl, &(object->list_compl_it));
   gtk_tree_selection_select_iter (selection, &(object->list_compl_it));

   g_signal_handler_unblock (G_OBJECT(object->tree_compl),
                             on_cursor_changed_handler);

   // completion has been created, now use the 1st item from TreeView
   object->pos_in_text = gtk_editable_get_position (GTK_EDITABLE (object));
   complete_from_list (object, NULL);

   g_list_free_full (WordList, g_free);
   g_list_free_full (FileList, g_free);
   return;
}


GtkWidget * gtk_completion_line_new()
{
   return GTK_WIDGET(g_object_new(gtk_completion_line_get_type(), NULL));
}


static void up_history (GtkCompletionLine* cl)
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

static void down_history (GtkCompletionLine* cl)
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
   cl->hist_search_mode = FALSE;
   int i;
   for (i = 0; i < MAX_HISTWORD_CHARS; i++) {
      cl->hist_word[i] = 0;
   }
   g_signal_emit_by_name (G_OBJECT(cl), "search_mode");
   history_unset_current (cl->hist);
}


static void search_history (GtkCompletionLine* cl, int next)
{ // must only be called if cl->hist_search_mode = TRUE
   /* a key is pressed and added to cl->hist_word */
   searching_history = TRUE;
   if (cl->hist_word[0])
   {
      const char * history_current_item = NULL;
      const char * search_str = cl->hist_word;
      if (next) {
         history_current_item = history_search_next_func (cl->hist);
      } else {
         history_current_item = history_search_first_func (cl->hist);
      }

      int search_str_len = 0;
      int search_match_start = cl->hist_search_match_start;
      if (search_match_start) {  /* ! */
         search_str_len = strlen (search_str);
      }

      while (1)
      {
         const char * s = NULL;
         if (history_current_item) {
            if (search_match_start) { /* ! */
               if (strncmp (history_current_item, search_str, search_str_len) == 0) {
                  s = history_current_item;
               }
            } else { /* CTRL-R / CTRL-S */
               s = strstr (history_current_item, search_str);
            }
         }
         if (s) {
            gtk_entry_set_text (GTK_ENTRY(cl), history_current_item);
            g_signal_emit_by_name (G_OBJECT(cl), "search_letter");
            return;
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
}

static guint tab_pressed(GtkCompletionLine* cl)
{
#ifdef DEBUG
   printf ("tab_pressed\n");
#endif
   if (cl->hist_search_mode == TRUE) {
      search_off(cl);
   }
   if (cl->win_compl) {
      // completion window exists, avoid calling complete_line()
      gboolean valid = gtk_tree_model_iter_next (cl->sort_list_compl, &(cl->list_compl_it));
      if(!valid) {
         gtk_tree_model_get_iter_first (cl->sort_list_compl, &(cl->list_compl_it));
      }
      complete_from_list (cl, NULL);
   } else { // complete_line ()
      complete_line (cl);
   }
   timeout_id = 0;
   return FALSE;
}

static void destroy_completion_window (GtkCompletionLine *cl)
{
   if (on_cursor_changed_handler) {
      g_signal_handler_block (G_OBJECT (cl->tree_compl), on_cursor_changed_handler);
   }
   gtk_list_store_clear (cl->list_compl);
   gtk_widget_destroy (cl->tree_compl);
   gtk_widget_destroy (cl->win_compl);
   cl->list_compl = NULL;
   cl->tree_compl = NULL;
   cl->win_compl = NULL;
   on_cursor_changed_handler = 0;
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
         valid = gtk_tree_model_iter_previous(cl->sort_list_compl, &(cl->list_compl_it));
         if(!valid) {
            int rowCount = gtk_tree_model_iter_n_children (cl->sort_list_compl, NULL);
            gtk_tree_model_iter_nth_child(cl->sort_list_compl, &(cl->list_compl_it), NULL, rowCount - 1);
         }
         complete_from_list (cl, NULL);
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
         complete_from_list (cl, NULL);
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
      case GDK_KEY_P:
      case GDK_KEY_p:
         if (key == GDK_KEY_p || key == GDK_KEY_P) {
            if (!(event->state & GDK_CONTROL_MASK)) {
               goto ordinary;
            }
         }
         if (cl->win_compl != NULL) {
            gboolean valid;
            valid = gtk_tree_model_iter_previous(cl->sort_list_compl, &(cl->list_compl_it));
            if(!valid) {
               int rowCount = gtk_tree_model_iter_n_children (cl->sort_list_compl, NULL);
               gtk_tree_model_iter_nth_child(cl->sort_list_compl, &(cl->list_compl_it), NULL, rowCount - 1);
            }
            complete_from_list (cl, NULL);
         } else {
            up_history(cl);
         }
         if (cl->hist_search_mode == TRUE) {
            search_off(cl);
         }
         return TRUE; /* stop signal emission */

      case GDK_KEY_space:
      {
         if (cl->hist_search_mode) {
            search_off (cl);
         }
         if (cl->win_compl != NULL) {
            destroy_completion_window (cl);
         }
      }
      return FALSE;

      case GDK_KEY_Down:
      case GDK_KEY_N:
      case GDK_KEY_n:
         if (key == GDK_KEY_n || key == GDK_KEY_N) {
            if (!(event->state & GDK_CONTROL_MASK)) {
               goto ordinary;
            }
         }
         if (cl->win_compl != NULL) {
            gboolean valid = gtk_tree_model_iter_next(cl->sort_list_compl, &(cl->list_compl_it));
            if(!valid) {
               gtk_tree_model_get_iter_first(cl->sort_list_compl, &(cl->list_compl_it));
            }
            complete_from_list (cl, NULL);
         } else {
            down_history(cl);
         }
         if (cl->hist_search_mode == TRUE) {
            search_off(cl);
         }
         return TRUE; /* stop signal emission */

      case GDK_KEY_Return:
         if (cl->win_compl != NULL) {
            destroy_completion_window (cl);
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
               cl->hist_search_match_start = FALSE;
               cl->hist_word[0] = 0;
               cl->hist_word_count = 0;
               g_signal_emit_by_name(G_OBJECT(cl), "search_mode");
            } else {
               // search next result for `cl->hist_word`
               search_history (cl, 1);
            }
            return TRUE; /* stop signal emission */
         } else goto ordinary;

      case GDK_KEY_exclam:
         if (cl->hist_search_mode == FALSE) {
           const char * entry_text = gtk_entry_get_text (GTK_ENTRY (cl));
           if (!*entry_text) {
              history_search_first_func = history_last;
              history_search_next_func  = history_prev;
              cl->hist_search_mode = TRUE;
              cl->hist_search_match_start = TRUE;
              cl->hist_word[0] = 0;
              cl->hist_word_count = 0;
              g_signal_emit_by_name (G_OBJECT(cl), "search_mode");
              return TRUE; /* stop signal emission */ 
           }
         }
         goto ordinary;

      case GDK_KEY_BackSpace:
         if (cl->hist_search_mode == TRUE) {
            if (cl->hist_word[0]) {
               cl->hist_word_count--;
               cl->hist_word[cl->hist_word_count] = 0;
               search_history (cl, 0);
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
            destroy_completion_window (cl);
         } else {
            // user cancelled
            g_signal_emit_by_name(G_OBJECT(cl), "cancel");
         }
         return TRUE; /* stop signal emission */

      case GDK_KEY_G:
      case GDK_KEY_g:
         if ((event->state & GDK_CONTROL_MASK) && cl->hist_search_mode) {
            search_off(cl);
            gtk_entry_set_text (GTK_ENTRY (cl), "");
            return TRUE; /* stop signal emission */
         } else goto ordinary;

      ordinary:
      default:
         if (cl->win_compl != NULL) {
            destroy_completion_window (cl);
         }
         if (cl->hist_search_mode == TRUE) {
            // https://developer.gnome.org/gdk2/stable/gdk2-Event-Structures.html#GdkEventKey
            if (event->state & GDK_CONTROL_MASK)
               return TRUE; /* stop signal emission */
            if (event->length > 0) {
               // event->string = char: 'c' 'd'
               cl->hist_word[cl->hist_word_count] = event->string[0];
               cl->hist_word_count++;
               if (cl->hist_word_count < MAX_HISTWORD_CHARS) {
                  search_history (cl, 0);
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
            if (isprint (*event->string)) {
               timeout_id = g_timeout_add (cl->tabtimeout,
                     (GSourceFunc) tab_pressed, cl);
            }
         }
         break;
   } //switch
   return FALSE;
}

