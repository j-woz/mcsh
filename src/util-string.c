
#include "mcsh.h"

#include "util-string.h"

char*
list_array_join_strings(list_array* L, char* delimiter)
{
  printf("join_strings: %zi\n", L->size);
  buffer B;
  buffer_init(&B, L->size);
  for (size_t i = 0; i < L->size; i++)
  {
    char* s = L->data[i];
    printf("append: '%s'\n", s);
    buffer_cat(&B, s);
    if (i < L->size - 1)
      buffer_cat(&B, delimiter);
  }
  char* result = buffer_dup(&B);
  printf("result: '%s'\n", result);
  buffer_finalize(&B);
  return result;
}

char*
list_array_join_values(list_array* L, char* delimiter)
{
  printf("join_values: %zi\n", L->size);
  buffer B;
  buffer_init(&B, L->size);
  for (size_t i = 0; i < L->size; i++)
  {
    mcsh_value* v = L->data[i];
    mcsh_resolve(v);
    // printf("append: '%s'\n", v->text);
    buffer_cat(&B, v->string);
    if (i < L->size - 1)
      buffer_cat(&B, delimiter);
  }
  char* result = buffer_dup(&B);
  printf("result: '%s'\n", result);
  buffer_finalize(&B);
  return result;
}
