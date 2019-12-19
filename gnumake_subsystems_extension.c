/*
 * File: gnumake_subsystems_extension.c
 * Author: Andrey P. Vasilyev <vap@vap.name>
 * Created: Sun Dec 15 10:05:00 2019
 * $Id$
 *
 * License: GPL 3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#include <gnumake.h>

int plugin_is_GPL_compatible;



// https://www.lemoda.net/c/va-copy/
// http://www.cplusplus.com/reference/cstdarg/va_copy/
static char *heap_vsprintf (const char *fmt, va_list args) {
  int calculated_size;
  int result_size;
  va_list args_copy;

  va_copy(args_copy, args);
  calculated_size = vsnprintf (NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (calculated_size < 0) {
    // ...         
    return NULL;
  }

  char *str = malloc (calculated_size + 1 /* for terminating 0 */);
  if (str == NULL) {
    // ...         
    return NULL;
  }

  result_size = vsnprintf (str, calculated_size + 1, fmt, args);

  if ((result_size < 0) || (result_size > calculated_size)) {
    free (str);
    return NULL;
  }

  return str;
}

static char *gmk_expand_fmt (const char *fmt, ...) {
  va_list args;
  char *formatted;
  va_start (args, fmt);
  formatted = heap_vsprintf (fmt, args);
  va_end (args);

  if (formatted == NULL) {
    // ...     
    return NULL;
  }

  char *result = gmk_expand (formatted);

  free (formatted);

  return result;
}

static void gmk_eval_fmt (const char *fmt, ...) {
  va_list args;
  char *formatted;
  va_start (args, fmt);
  formatted = heap_vsprintf (fmt, args);
  va_end (args);

  if (formatted == NULL) {
    // ...     
    return;
  }

  gmk_floc floc;
  // TODO: What should I pass here?
  floc.filenm = "<gmk_eval_fmt>";   
  floc.lineno = 0;        

  gmk_eval (formatted, &floc);

  free (formatted);
}

static char *retrieve_first_word (const char **str) {
  while ((**str) && isspace (**str))
    ++*str;
  const char *word_start = *str;
  while ((**str) && (!isspace (**str)))
    ++*str;
  const char *word_end = *str;

  if ((*word_start == 0) || (word_end == word_start))
    return 0;

  size_t word_len = word_end - word_start;

  char *result = (char *)gmk_alloc (word_len + 1);
  if (result == NULL) {
    // ...         
    return NULL;
  }

  memcpy (result, word_start, word_len);
  result [word_len] = 0;
  return result;
}


static char *from_here (const char *nm, unsigned int argc, char **argv) {
  const char *filename_list = argv [0];

  char *here = NULL;
  char *result = NULL;

  while (1) {
    char *filename = retrieve_first_word (&filename_list);
    if (filename == NULL)
      break;

    if (!strncmp (filename, "/", 1)) {
      // Absolute path
      char *old_result = result;
      if (old_result) {
        result = gmk_expand_fmt ("%s $(abspath %s)", old_result, filename);
        gmk_free (old_result);
      } else {
        result = gmk_expand_fmt ("$(abspath %s)", filename);
      }
    } else {
      // Relative path
      if (here == NULL) {
        char *here_undefined = gmk_expand ("$(filter undefined,$(origin here))");
        if ((here_undefined != NULL) && (!strcmp (here_undefined, "undefined"))) {
          // HERE is undefined, need to get from MAKEFILE_LIST
          here = gmk_expand ("$(abspath $(dir $(firstword $(MAKEFILE_LIST))))");
        } else {
          here = gmk_expand ("$(abspath $(here))");
        }
        if (here_undefined)
          gmk_free (here_undefined);
      }
      char *old_result = result;
      if (old_result) {
        result = gmk_expand_fmt ("%s $(abspath %s/%s)", old_result, here, filename);
        gmk_free (old_result);
      } else {
        result = gmk_expand_fmt ("$(abspath %s/%s)", here, filename);
      }
    }

    gmk_free (filename);
  }

  if (here != NULL)
    gmk_free (here);

  return result;
}


static char *include_subsystem (const char *nm, unsigned int argc, char **argv) {
  char *filename = argv [0];
  char *filename_from_here = gmk_expand_fmt ("$(from-here %s)", filename);
  gmk_eval_fmt ("here_stack:=$(here_stack) $(here)");
  gmk_eval_fmt ("here:=$(abspath $(dir %s))", filename_from_here);

  char *found = gmk_expand_fmt ("$(filter %s,$(past_includes))", filename_from_here);
  if ((found == NULL) || (0 == strlen (found))) {
    gmk_eval_fmt ("past_includes:=$(sort $(past_includes) %s)", filename_from_here);
    gmk_eval_fmt ("include %s", filename_from_here);
  }
  if (found)
    gmk_free (found);

  gmk_eval_fmt ("here:=$(lastword $(here_stack))");
  gmk_eval_fmt ("here_stack:=$(filter-out $(here),$(here_stack))");
  gmk_free (filename_from_here);
}


int gnumake_subsystems_extension_gmk_setup (void) {
  gmk_add_function ("from-here", from_here, 1, 1, GMK_FUNC_DEFAULT);
  gmk_add_function ("include-subsystem", include_subsystem, 1, 1, GMK_FUNC_DEFAULT);
  return 1; // success
}
