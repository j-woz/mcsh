
#pragma once

#include <stdbool.h>

/*
  LOOKUP H

  Read-only human-readable lookup data structure.
  Codes must be > 0 (like enums).
  The compiler should check that the entry count is correct
  and that the strings are smaller than LOOKUP_SIZE.

  Use like:

  lookup_entry L[4] =
    {{MCSH_FN_NORMAL,  "function"},
     {MCSH_FN_INPLACE, "inplace"},
     {MCSH_FN_MACRO,   "macro"},
     lookup_sentinel
    };
  int t = lookup_by_text(L, name);
  valgrind_assert(t >= 0);

  OR:

  lookup_entry L[7] =
    {{MCSH_THING_MODULE, "MODULE"},
     {MCSH_THING_BLOCK,  "BLOCK" },
     {MCSH_THING_SUBFUN, "SUBFUN"},
     {MCSH_THING_SUBCMD, "SUBCMD"},
     {MCSH_THING_STMT,   "STMT"  },
     {MCSH_THING_TOKEN,  "TOKEN" },
     lookup_sentinel
    };
  char* t = lookup_by_code(L, thing->type);
  valgrind_assert_msg(t != NULL,
                      "thing_str(): unknown thing->type: %i\n",
                      thing->type);
*/

#define LOOKUP_SIZE 64

typedef struct
{
  int  code;
  char text[LOOKUP_SIZE];
} lookup_entry;

static lookup_entry lookup_sentinel = {-1, "sentinel"};

/** @return the code for the matching string or a negative int */
static inline int
lookup_by_text(lookup_entry* L, const char* name)
{
  int i = 0;
  int result = -1;  // not found
  while (true)
  {
    lookup_entry* e = &L[i];
    if (e->code < 0)
      break;  // sentinel
    // printf("text: '%s'\n", e->text);
    if (strcmp(e->text, name) == 0)
    {
      result = e->code;
      break;
    }
    i++;
  }
  return result;
}

/** @return a pointer to the string for the matching code or NULL */
static inline char*
lookup_by_code(lookup_entry* L, int code)
{
  int i = 0;
  char* result = NULL;  // not found
  while (true)
  {
    lookup_entry* e = &L[i];
    if (e->code < 0)
      break;  // sentinel
    if (code == e->code)
    {
      result = &e->text[0];
      break;
    }
    i++;
  }
  return result;
}
