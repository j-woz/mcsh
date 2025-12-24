
#include <stdio.h>

#include "mcsh-preprocess.h"

bool
mcsh_script_preprocess(char* code)
{
  bool quote = false;
  // Was the prior character whitespace?  Needed for hash comments.
  bool prev_ws = true;

  for (int i = 0; code[i] != '\0'; i++)
  {
    if (code[i] == '"')
    {
      quote = !quote;
      continue;
    }
    // Backslash line wrapping
    if (code[i] == '\\')
    {
      if (code[i+1] == '\n')
      {
        code[i]   = ' ';
        code[i+1] = ' ';
      }
      i++;
      continue;
    }
    // Hash comment   # ...
    if (code[i] == '#' && prev_ws)
    {
      for ( ; code[i] != '\n' && code[i] != '\0'; i++)
        code[i] = ' ';
      prev_ws = true;
    }
    // C-style comment  /* ... */
    if (code[i] == '/' && code[i+1] == '*')
    {
      for ( ; ! (code[i] == '*' && code[i+1] == '/') &&
                 code[i] != '\0';
           i++)
        code[i] = ' ';
      code[i++] = ' ';
      code[i]   = ' ';
    }
    // C++-style comment  // ...
    if (code[i] == '/' && code[i+1] == '/')
    {
      for ( ; ! (code[i] == '\n') && code[i] != '\0'; i++)
        code[i] = ' ';
    }
    prev_ws = code[i] == ' ' || code[i] == '\t' || code[i] == '\n';
  }
  return true;
}
