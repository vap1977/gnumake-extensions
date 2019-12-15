/*
 * File: include_once_extension.c
 * Author: Andrey P. Vasilyev <vap@vap.name>
 * Created: Sun Dec 15 19:57:51 2019
 * $Id$
 *
 * License: GPL
 */

/*

  This gnumake loadable module is intended to eliminate the need for
  "include guards" in included sub-makefiles.

  Usage:

  include_once_extension.so: include_once_extension.c
          $(CC) -shared -fPIC -o $@ $<
  ...
  load include_once_extension.so
  ...
  $(include-once ../subsystem/subsystem.mk)


  Reading list:
  * https://en.wikipedia.org/wiki/Include_guard - similar problem in C and C++
  * https://www.gnu.org/software/make/manual/html_node/Loading-Objects.html

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnumake.h>

int plugin_is_GPL_compatible;


struct include_once_elem {
  struct include_once_elem *next;
  const char *fullname_of_already_included;
};

static struct include_once_elem *include_once_list = 0;

// 0 - error
static int include_once_add_already_included (const char *name) {
  struct include_once_elem *elem = (struct include_once_elem *)malloc (sizeof (*elem));
  if (elem == NULL)
    return 0;

  elem->fullname_of_already_included = strdup (name);
  if (elem->fullname_of_already_included == NULL) {
    free (elem);
    return 0;
  }
  elem->next = include_once_list;
  include_once_list = elem;
  return 1;
}

static int include_once_was_already_included (const char *name) {
  for (struct include_once_elem *elem = include_once_list;
       elem != NULL;
       elem = elem->next) {
    //fprintf (stderr, "DEBUG: Testing: \"%s\"\n", elem->fullname_of_already_included);   
    if (!strcmp (elem->fullname_of_already_included, name))
      return 1;
  }
  return 0;
}

static char *include_once_do_abspath (const char *name) {
  size_t include_cmd_abspath_cmd_len = strlen ("$(abspath )") + strlen (name) + 1;
  char *include_cmd_abspath_cmd = (char *)malloc (include_cmd_abspath_cmd_len);
  if (include_cmd_abspath_cmd == NULL) {
    // No memory
    return 0;
  }
  snprintf (include_cmd_abspath_cmd, include_cmd_abspath_cmd_len,
            "$(abspath %s)", name);
  char *result = gmk_expand (include_cmd_abspath_cmd);
  free (include_cmd_abspath_cmd);

  if (result == NULL) {
    // ...        
  }

  // User must free the result by calling gmk_free ()
  return result;
}

static void include_once_do_include (const char *file_to_include, int debug_mode) {

  char *file_to_include_abspath = include_once_do_abspath (file_to_include);
  if (file_to_include_abspath == NULL) {
    if (debug_mode)
      fprintf (stderr, "DEBUG: include_once: no memory\n");
    return;
  }

  if (debug_mode)
    fprintf (stderr, "DEBUG: include_once (\"%s\", abspath \"%s\")\n",
             file_to_include, file_to_include_abspath);

  if (!include_once_was_already_included (file_to_include_abspath)) {
    if (debug_mode)
      fprintf (stderr, "DEBUG: Really including: \"%s\"\n", file_to_include);

    if (!include_once_add_already_included (file_to_include_abspath)) {
      // Error
      // TODO: How can I communicate error with "make"?   
      gmk_free (file_to_include_abspath);
      return;
    }

    gmk_floc floc;
    // TODO: What should I pass here?
    floc.filenm = "test";   
    floc.lineno = 0;        

    size_t include_cmd_len = strlen ("include ") + strlen (file_to_include) + 1;
    char *include_cmd = (char *)malloc (include_cmd_len);
    if (include_cmd == NULL) {
      // No memory
      // TODO: How can I communicate error with "make"?   
      gmk_free (file_to_include_abspath);
      return;
    }
    snprintf (include_cmd, include_cmd_len, "include %s", file_to_include);
    gmk_eval (include_cmd, &floc);
    free (include_cmd);
  } else {
    if (debug_mode)
      fprintf (stderr, "DEBUG: Was already included: \"%s\" (abspath: \"%s\")\n",
               file_to_include, file_to_include_abspath);
  }
  gmk_free (file_to_include_abspath);
}

static char *include_once (const char *nm, unsigned int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    char *file_to_include = argv [i];
    include_once_do_include (file_to_include, 0 /* debug_mode */);
  }
  return NULL;
}

static char *include_once_debug (const char *nm, unsigned int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    char *file_to_include = argv [i];
    include_once_do_include (file_to_include, 1 /* debug_mode */);
  }
  return NULL;
}

int include_once_extension_gmk_setup (void) {
  gmk_add_function ("include-once", include_once, 1, 0, GMK_FUNC_DEFAULT);
  gmk_add_function ("include-once-debug", include_once_debug, 1, 0, GMK_FUNC_DEFAULT);
  return 1; // success
}
