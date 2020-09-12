
/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

/*
 * Generic implementation of HistoryFile
 */

#ifndef __HISTORY_H
#define __HISTORY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Whistory HistoryFile;

/// create a new HistoryFile
/// the history is initialized using filename and the filename is stored
/// maxcount > 0 sets a maximun item count
HistoryFile * history_new (char * filename, unsigned int maxcount);

/// history is saved to file
void history_save (HistoryFile * history);

/// history is destroyed, you must variable to NULL
void history_destroy (HistoryFile * history);

/// clear history and reload from file
void history_reload (HistoryFile * history);

/// print history, good for debugging
void history_print (HistoryFile * history);

/// some apps might want to handle prev/next in a special way
void history_unset_current (HistoryFile * history);

/// returns string with the current history item
const char * history_get_current (HistoryFile * history);

/// returns current index: valid index > 0
/// assume unser current item if index <= 0
int history_get_current_index (HistoryFile * history);

/// moves position to the next entry and returns text
/// if there is no next entry, the position is not moved and returns NULL
const char * history_next (HistoryFile * history);

/// moves position to the previous entry and returns text
/// if there is no next entry, the position is not moved and returns NULL
const char * history_prev (HistoryFile * history);

/// moves position to the first entry en returns text
/// if there are no entries, it returns NULL
const char * history_first (HistoryFile * history);

/// moves position to the last entry en returns text
/// if there are no entries, it returns NULL
const char * history_last (HistoryFile * history);

/// append item to history
void history_append (HistoryFile * history, const char * text);

#ifdef __cplusplus
}
#endif

#endif
