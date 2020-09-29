/*****************************************************************************
 *  $Id: gtkcompletionline.h,v 1.12 2003/11/16 10:43:32 andreas99 Exp $
 *  Copyright (C) 2000, Mihai Bazon "Mishoo" <mishoo@fenrir.infoiasi.ro>
 *
 *   Distributed under the terms of the GNU General Public License. You are
 *  free to use/modify/distribute this program as long as you comply to the
 *    terms of the GNU General Public License, version 2 or above, at your
 *      option, and provided that this copyright notice remains intact.
 *****************************************************************************/


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

struct _GtkCompletionLine
{
   GtkEntry parent;
   GtkWidget * win_compl;
   GtkListStore * list_compl;
   GtkTreeModel * sort_list_compl;
   GtkWidget    * tree_compl;
   GtkTreeIter    list_compl_it;
   int pos_in_text; /* Cursor position in main "line" */
 
   GList * cmpl;  /* Completion list */
   GList * where; /* current row pointer ??? */

   HistoryFile * hist;
   gboolean hist_search_mode;
   char hist_word[1024]; /* history search: word that is being typed */
   int hist_word_count;  /* history search: word that is being typed */

   int first_key;
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

#ifdef __cplusplus
}
#endif

#endif /* __GTKCOMPLETIONLINE_H__ */

