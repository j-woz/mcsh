
#include <stdlib.h>

#include "list-array.h"

void
list_array_increase(list_array* L)
{
  size_t capacity_new;
  if (L->capacity == 0)
    capacity_new = 1;
  else
    capacity_new = L->capacity * 2;
  L->data = realloc_checked(L->data, capacity_new * sizeof(void*));
  L->capacity = capacity_new;
}
