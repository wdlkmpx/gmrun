/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

#ifndef __CONFIG_PREFS_H
#define __CONFIG_PREFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "gtkcompat.h" // glib-compat.h
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_init ();
void config_destroy ();
void config_reload ();
void config_print ();

gboolean config_get_int (const char * key, int * out_int);

/// changes string pointer (must not be freed)
gboolean config_get_string (const char * key, char ** out_str);

/// allocates a string that must be freed with g_free
gboolean config_get_string_expanded (const char * key, char ** out_str);

/// returns a constant string (must not be freed)
char * config_get_handler_for_extension (const char * extension);

#ifdef __cplusplus
}
#endif

#endif
