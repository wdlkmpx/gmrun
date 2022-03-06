/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

/*
 * Read config file formatted like this:
 *
 * key1 = value1
 * key2=value2
 *
 * It's possible to open several files with the same config
 * The keys will be updated with thew new values.
 * 'key' is not case sensitive
 */

#include "config_prefs.h"

#define APP_CONFIG_FILE "gmrunrc"

// ============================================================
//                        PRIVATE
// ============================================================

static GList * PrefsGList = NULL;
static GList * ExtensionGList = NULL;

struct _pref_item
{
   char * key;
   char * value;
};
typedef struct _pref_item pref_item;


static void pref_item_free (pref_item * item)
{
   if (item) {
      if (item->key)   free (item->key);
      if (item->value) free (item->value);
      free (item);
   }
}

static void pref_item_free_cb (gpointer data)
{
   pref_item_free ((pref_item *) data);
}


static GList * config_find_key (GList * list, const char * key)
{
   GList *i;
   pref_item * item;

   if (!key || !*key) { /* ignore empty keys (strings) */
      return (NULL);
   }

   for (i = list; i; i = i->next)
   {
      item = (pref_item *) (i->data);
      if (strcasecmp (key, item->key) == 0) {
         return (i);
      }
   }
   return (NULL); /* key not found */
}


static void config_replace_key (GList ** list, pref_item * item)
{
   GList * found = config_find_key (*list, item->key);
   if (found) {
      /* only update found item */
      pref_item * found_item = (pref_item *) (found->data);
      if (strcmp (found_item->value, item->value) == 0) {
         pref_item_free (item);
         return; /* values are equal, nothing to update */
      }
      free (found_item->value);
      found_item->value = strdup (item->value);
      pref_item_free (item);
   } else {
      /* append item */
      *list = g_list_append (*list, (gpointer) item);
   }
}


/** get value, it's always a string **/
static char * config_get_item_value (GList * list, const char * key)
{
   GList * ret;
   pref_item * item;

   ret = config_find_key (list, key);
   if (ret) {
      item = (pref_item *) (ret->data);
      return (item->value);
   }
   return (NULL); /* key not found */
}


static void config_load_from_file (const char * filename, GList ** out_list)
{
   FILE *fp;
   char buf[1024];

   char * stripped;
   char ** keyvalue;
   pref_item * item;

   fp = fopen (filename, "r");
   if (!fp) {
      return;
   }

   /* Read file line by line */
   while (fgets (buf, sizeof (buf), fp))
   {
      stripped = buf;
      while (*stripped && *stripped <= 0x20) { // 32 = space
         stripped++;
      }
      if (strlen (stripped) < 3 || *stripped == '#') {
         continue;
      }
      if (!strchr (stripped, '=')) {
         continue;
      }

      item = (pref_item *) calloc (1, sizeof (pref_item));
      keyvalue = g_strsplit (stripped, "=", 2);
      item->key   = g_strstrip (keyvalue[0]);
      item->value = g_strstrip (keyvalue[1]);

      if (!*item->key || !*item->value) {
         g_strfreev (keyvalue);
         free (item);
         continue;
      }

      /// fprintf (stderr, "### %s = %s\n", key, value);
      /* Insert or replace item */
      config_replace_key (out_list, item);
    }

    fclose (fp);
    return;
}


static void create_extension_handler_list (void)
{
   GList * i;
   pref_item * item, * item_out;
   char ** str_vector;

   for (i = PrefsGList; i; i = i->next)
   {
      item = (pref_item *) (i->data);
      if (strncasecmp (item->key, "EXT:", 4) == 0)
      {
         int w;
         str_vector = g_strsplit (item->key + 4, ",", 0);
         for (w = 0; str_vector[w]; w++)
         {
            item_out = (pref_item *) calloc (1, sizeof (pref_item));
            item_out->key   = strdup (str_vector[w]);
            item_out->value = strdup (item->value);
            config_replace_key (&ExtensionGList, item_out);
         }
         g_strfreev (str_vector);
      }
   }
}


static char * replace_variable (char * txt) /* config_get_string_expanded() */
{
   // pre${variable}post  : ${Terminal} -e ...
   char * pre = NULL, * post = NULL;
   char * variable = NULL;
   char * variable_value = NULL;
   char * new_text = NULL;
   char * p, * p2;

   if (strlen (txt) < 5) { // at least ${xx}
      return (NULL);
   }
   p  = strstr (txt, "${");
   if (!p) {
      return (NULL);  // syntax error
   }
   if (!strchr (p + 3, '}')) {
      return (NULL);  // syntax error
   }

   if (txt[0] != '$' && txt[1] != '$') {
      pre = strdup (txt);
      p2 = strchr (pre, '$');
      if (p2) p2 = 0;
   }

   variable = strdup (p + 2); // variable start
   p2 = strchr (variable, '}'); // variable end
   *p2 = 0;                     // `Terminal`
   post = p2 + 1;               // ` -e ...`

   variable_value = config_get_item_value (PrefsGList, variable); // xterm

   if (variable_value) {
      if (pre) {
         // pre xterm -e ...
         new_text = g_strconcat (pre, variable_value, post, NULL);
      } else {
         // xterm -e ...
         new_text = g_strconcat (variable_value, post, NULL);
      }
   }

   if (pre)      free (pre);
   if (variable) free (variable);

   return (new_text);
}


// ============================================================
//                     PUBLIC
// ============================================================

void config_init ()
{
   if (PrefsGList) {
      return;
   }

   char * config_file = g_build_filename ("/etc", APP_CONFIG_FILE, NULL);
   config_load_from_file (config_file, &PrefsGList);
   g_free (config_file);

#ifdef FOLLOW_XDG_SPEC
   config_file = g_build_filename (g_get_user_config_dir(), APP_CONFIG_FILE, NULL);
#else
   config_file = g_build_filename (g_get_home_dir(), "." APP_CONFIG_FILE, NULL);
#endif
   config_load_from_file (config_file, &PrefsGList);
   g_free (config_file);

   create_extension_handler_list ();
}


void config_destroy ()
{
   if (PrefsGList) {
      g_list_free_full (PrefsGList,     pref_item_free_cb);
      g_list_free_full (ExtensionGList, pref_item_free_cb);
      PrefsGList     = NULL;
      ExtensionGList = NULL;
   }
}


void config_reload ()
{
   config_destroy ();
   config_init ();
}


void config_print ()
{
   GList * i;
   pref_item * item;
   for (i = PrefsGList; i; i = i->next)
   {
      item = (pref_item *) (i->data);
      printf ("%s = %s\n", item->key, item->value);
   }
   for (i = ExtensionGList; i; i = i->next)
   {
      item = (pref_item *) (i->data);
      printf ("%s = %s\n", item->key, item->value);
   }
}


gboolean config_get_int (const char * key, int * out_int)
{
   char * value;
   value = config_get_item_value (PrefsGList, key);
   if (value) {
      *out_int = (int) strtoll (value, NULL, 0);
      return TRUE;
   } else {
      *out_int = -1;
      return FALSE;
   }
}


// returns a string that must be freed with g_free
gboolean config_get_string_expanded (const char * key, char ** out_str)
{
   char * value1, * value2, * value = NULL;

   value1 = config_get_item_value (PrefsGList, key);
   if (value1 && strstr (value1, "${")) {
      value2 = replace_variable (value1);
      value = value2;
      // expand variable up to 2 times
      if (value2 && strstr (value2, "${")) {
         value = replace_variable (value2);
         free (value2);
      }
   } else if (value1) {
      value = strdup (value1);
   }

   if (value) {
      *out_str = value;
      return TRUE;
   } else {
      *out_str = NULL;
      return FALSE;
   }
}


gboolean config_get_string (const char * key, char ** out_str)
   {
   char * value;
   value = config_get_item_value (PrefsGList, key);
   if (value) {
      *out_str = value;
      return TRUE;
   } else {
      *out_str = NULL;
      return FALSE;
   }
}


char * config_get_handler_for_extension (const char * extension)
{
   char * handler;
   if (extension && *extension == '.') {
      extension++; // .html -> html
   }
   handler = config_get_item_value (ExtensionGList, extension);
   return (handler);
}
