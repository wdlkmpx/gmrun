/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

/*
 * - Generic implementation of HistoryFile
 * - Loads history from file. Saves history to file.
 * - Keeps track of the last entry, making it a bit more efficient
 * - Only adding entries is supported ...
 * - Supports maximum number of entries,
 *     The first entry is removed if the new entry exceeds the max count.
 * - Does not allow duplicate strings
 *     This is not a good idea if the list is too long
 * - If entry exists it's moved to the end of the list..
 * 
 * HOWTO: see example at the end of the file
 */

#include "history.h"

// ============================================================
//                        PRIVATE
// ============================================================

struct _Whistory
{
   long int index;
   unsigned int count;
   unsigned int max;
   char  * filename;
   int has_changed;
   int save_if_changed;
   GList * list;
   GList * list_end;
   GList * current;
};

static void _history_clear (HistoryFile * history)
{
   // keep history->filename (destroyed in _history_free())
   if (history->list) {
      g_list_free_full (history->list, g_free);
      history->list = NULL;
      history->has_changed = 1;
   }
   history->index = 0;
   history->count = 0;
}


static void _history_free (HistoryFile * history)
{
   _history_clear (history);
   if (history->filename) {
      g_free (history->filename);
      history->filename = NULL;
   }
   g_free (history);
}


/// load entries from file and initialize private variables
static void _history_load_from_file (HistoryFile * history, const char * filename)
{
   FILE *fp;
   char buf[1024];
   char * p, * item;
   size_t len;
   unsigned int count = 0;
   unsigned int max   = history->max;
   GList * out_list = NULL;

   fp = fopen (filename, "r");
   if (!fp) {
      return;
   }

   /* Read file line by line */
   while (fgets (buf, sizeof (buf), fp))
   {
      p = buf;
      while (*p && *p <= 0x20) { // 32 = space [ignore spaces]
         p++;
      }
      if (!*p) {
         continue;
      }

      item = g_strdup (p);
      len = strlen (buf);
      item[len-1] = 0;
      if (max > 0 && count >= max) {
         break;
      }
      count++;
      out_list = g_list_prepend (out_list, (gpointer) item);
   }

   if (out_list) {
      history->list_end = out_list;
      history->list     = out_list;
      if (out_list->next) {
         history->list  = g_list_reverse (out_list);
      }
      history->index    = 1;
   }

   history->count   = count;
   history->current = history->list; // current = 1st item
   fclose (fp);
   return;
}


void _history_write_to_file (HistoryFile * history, const char * filename)
{
   FILE *fp;
   fp = fopen (filename, "w");
   if (!fp) {
      return;
   }
   GList * i;
   for (i = history->list;  i;  i = i->next)
   {
      fprintf (fp, "%s\n", (char *) (i->data));
   }
   fclose (fp);
   return;
}


// ============================================================
//                     PUBLIC
// ============================================================

HistoryFile * history_new (char * filename, unsigned int maxcount)
{
   HistoryFile * history = g_malloc0 (sizeof (HistoryFile));
   history->max = maxcount;
   if (filename || *filename) {
      history->filename = g_strdup (filename);
      _history_load_from_file (history, filename);
   }
   return (history);
}

void history_save (HistoryFile * history, int save_if_changed)
{
   if (history && history->filename) {
      if (save_if_changed) {
         if (history->has_changed) {
            _history_write_to_file (history, history->filename);
         } // else printf ("not saving!!\n");
      } else {
         _history_write_to_file (history, history->filename);
      }
   } else {
      fprintf (stderr, "history_save(): history or filename is NULL\n");
   }
}

void history_destroy (HistoryFile * history)
{
   if (history) {
      _history_free (history);
   }
}

void history_reload (HistoryFile * history)
{
   if (history) {
      _history_clear (history);
      if (history->filename) {
         _history_load_from_file (history, history->filename);
      }
      history->has_changed = 0;
   }
}


void history_print (HistoryFile * history)
{
   if (history) {
      unsigned int count = 0;
      GList * i;
      for (i = history->list;  i;  i = i->next) {
         count++;
         printf ("[%d] %s\n", count, (char *) (i->data));
      }
      printf ("-- list internal count: [%d]\n", history->count);
      if (history->list_end) {
         printf ("** list     : %s\n", (char *) (history->list->data));
         printf ("** list_end : %s\n", (char *) (history->list_end->data));
      }
      if (history->current) {
         printf ("** list current : %s\n", (char *) (history->current->data));
      }
   }
}


// some apps might want to handle prev/next in a special way
void history_unset_current (HistoryFile * history)
{
   if (history) {
      history->current = NULL;
      history->index = -1;
   }
}


const char * history_get_current (HistoryFile * history)
{
   if (history) return ((char*) (history->current->data));
   else         return (NULL);
}


int history_get_current_index (HistoryFile * history)
{
   if (history) return (history->index);
   else         return (-1);
}


const char * history_next (HistoryFile * history)
{
   if (history->current && history->current->next) {
      history->current = history->current->next;
      history->index++;
      return ((char *) (history->current->data));
   }
   return (NULL);
}


const char * history_prev (HistoryFile * history)
{
   if (history->current && history->current->prev) {
      history->current = history->current->prev;
      history->index--;
      return ((char *) (history->current->data));
   }
   return (NULL);
}


const char * history_first (HistoryFile * history)
{
   if (history->list) {
      history->current = history->list;
      history->index = 1;
      return ((char *) (history->current->data));
   }
   return (NULL);
}


const char * history_last (HistoryFile * history)
{
   if (history->list) {
      history->current = history->list_end; // g_list_last (history->list);
      return ((char *) (history->current->data));
      history->index = history->count;
   }
   return (NULL);
}


void history_append (HistoryFile * history, const char * text)
{
   if (!text || !*text) {
      return;
   }

   GList * i, * templist;
   char * ientry;
   // if new entry = last entry, then abort
   if (history->list_end) {
      ientry = (char *) (history->list_end->data);
      if (strcmp (text, ientry) == 0) {
         return;
      }
   }
   // do not allow duplicate entries, remove existing entry
   for (i = history->list;  i;  i = i->next)
   {
      char * ientry = (char *) (i->data);
      if (strcmp (text, ientry) == 0) {
         // entry already exists.. remove
         templist = g_list_remove (i, i->data);
         if (!templist->prev) { // no previous entry.. new start
            history->list = templist;
         }
         if (!templist->next) { // no next... only 1 entry
            history->list_end = templist;
         }
         history->count--;
         break;
      }
      // TODO: handle 'current'
   }

   // if new entry exceeds the max count, first entry will be removed
   unsigned int remove_first = 0;
   if (history->max > 0
       && history->count > 1
       && history->count >= history->max) {
      remove_first = 1;
   }

   if (history->count < history->max) {
      history->count++;
   }

   // add new item
   char * new_item = g_strdup (text);
   GList * position = history->list_end ? history->list_end : history->list;
   position = g_list_append (position, (gpointer) new_item);
   history->has_changed = 1;
   if (!history->list) {
      history->list = position;         // first
      history->current = history->list; // set current to first
      history->index   = 1;
   }

   if (history->list_end) {
      history->list_end = history->list_end->next;
   } else {
      history->list_end = history->list; // first element has just been added
   }

   if (remove_first && history->list->next) {
      // problem when removing the first entry - current may become invalid
      if (history->current == history->list) {
         history->current = history->list->next;
      }
      gpointer data = history->list->data;
      history->list = g_list_remove (history->list, data);
   }
}


void history_reverse (HistoryFile * history)
{
   if (history && history->list) {
      GList * prev_first = history->list;
      history->list = g_list_reverse (history->list);
      history->list_end = prev_first;
      if (history->current == prev_first) {
         history->current = history->list;
      }
   }
}


// ============================================================
//                     example
// ============================================================

/*
// gcc -o history history.c -Wall -g -O2 `pkg-config --cflags glib-2.0` `pkg-config --libs glib-2.0`

int main (int argc, char ** argv)
{

   #define HISTORY_FILE ".history_sample"
   char * HOME = getenv ("HOME");
   char history_file[512] = "";
   if (HOME) {
      snprintf (history_file, sizeof (history_file), "%s/%s", HOME, HISTORY_FILE);
   }

   HistoryFile * history;
   history = history_new (history_file, 11);

   printf ("REVERSE\n");
   history_reverse (history);
   history_print (history);

   printf ("APPEND\n");
   history_append (history, "pegasus fantasy");
   history_print (history);

   history_append (history, "soldier dream");
   history_append (history, "soldier dream");
   history_append (history, "soldier dream");
   history_print (history);

   history_append (history, "blue dream");
   history_append (history, "soldier dream");
   history_print (history);

   history_save (history, HISTORY_SAVE_IF_CHANGED);
   history_destroy (history);
   history = NULL;
   return (0);
}
*/
