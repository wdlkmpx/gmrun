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

#ifndef __GTKCOMPLETIONLINE_H__
#define __GTKCOMPLETIONLINE_H__

#include "gtkcompat.h"
#include "history.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define GTK_COMPLETION_LINE(obj)    G_TYPE_CHECK_INSTANCE_CAST(obj, gtk_completion_line_get_type(), GtkCompletionLine)
#define GTK_COMPLETION_LINE_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, gtk_completion_line_get_type(), GtkCompletionLineClass)
#define IS_GTK_COMPLETION_LINE(obj)  GTK_CHECK_TYPE(obj, gtk_completion_line_get_type())

typedef struct _GtkCompletionLine GtkCompletionLine;
typedef struct _GtkCompletionLineClass GtkCompletionLineClass;

#define MAX_HISTWORD_CHARS 2048

struct _GtkCompletionLine
{
   GtkEntry parent;
   GtkWidget * win_compl;
   GtkListStore * list_compl;
   GtkTreeModel * sort_list_compl;
   GtkWidget    * tree_compl;
   GtkTreeIter    list_compl_it;
   int pos_in_text; /* Cursor position in main "line" */
 
   HistoryFile * hist;
   gboolean hist_search_mode;
   gboolean hist_search_match_start;
   char hist_word[MAX_HISTWORD_CHARS]; /* history search: word that is being typed */
   int hist_word_count;  /* history search: word that is being typed */

   int tabtimeout;
   int show_dot_files;
};


struct _GtkCompletionLineClass
{
   GtkEntryClass parent_class;
   /* add your CLASS members here */
   void (* unique)      (GtkCompletionLine *cl);
   void (* notunique)   (GtkCompletionLine *cl);
   void (* incomplete)  (GtkCompletionLine *cl);
   void (* runwithterm) (GtkCompletionLine *cl);
   void (* search_mode) (GtkCompletionLine *cl);
   void (* search_letter) (GtkCompletionLine *cl);
   void (* search_not_found) (GtkCompletionLine *cl);
   void (* ext_handler) (GtkCompletionLine *cl);
   void (* cancel)      (GtkCompletionLine *cl);
};


GType gtk_completion_line_get_type (void);
GtkWidget * gtk_completion_line_new ();
void gtk_completion_line_last_history_item (GtkCompletionLine*);

void compline_clear_selection (GtkCompletionLine* cl);

#ifdef __cplusplus
}
#endif

#endif /* __GTKCOMPLETIONLINE_H__ */

