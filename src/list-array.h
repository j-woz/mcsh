
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "util.h"

typedef struct
{
  void** data;
  size_t size;
  size_t capacity;
} list_array;

static inline void
list_array_init(list_array* L, size_t capacity_init)
{
  L->size = 0;
  L->capacity = capacity_init;
  if (L->capacity > 0)
    L->data = malloc_checked(capacity_init * sizeof(void*));
  else
    L->data = NULL;
}

static inline list_array*
list_array_construct(size_t capacity_init)
{
  list_array* L = malloc_checked(sizeof(list_array));
  list_array_init(L, capacity_init);
  return L;
}

static inline size_t
list_array_size(list_array* L)
{
  return L->size;
}

void list_array_increase(list_array* L);

static inline void
list_array_add(list_array* L, void* p)
{
  if (L->size == L->capacity)
    list_array_increase(L);
  L->data[L->size] = p;
  L->size++;
}

static inline void*
list_array_get(list_array* L, size_t i)
{
  return L->data[i];
}

static inline void
list_array_reset(list_array* L)
{
  L->size = 0;
}

static inline void
list_array_delete(list_array* L)
{
  for (size_t i = 0; i < L->size; i++)
    free(L->data[i]);
  L->size = 0;
}

static inline void
list_array_delete_callback(list_array* L,
                           void (*callback)(void*))
{
  for (size_t i = 0; i < L->size; i++)
    callback(L->data[i]);
  L->size = 0;
}

static inline void
list_array_clear(UNUSED list_array* L)
{
  // NYI: Cf. mcsh_stmt_free()
}

static inline void
list_array_finalize(list_array* L)
{
  // May be NULL if initialized to size 0:
  if (L->data != NULL)
    free(L->data);
}

static inline void
list_array_free(list_array* L)
{
  list_array_finalize(L);
  free(L);
}

static inline void
list_array_demolish(list_array* L)
{
  list_array_delete(L);
  list_array_finalize(L);
}

static inline void
list_array_demolish_callback(list_array* L,
                             void (*callback)(void*))
{
  list_array_delete_callback(L, callback);
  list_array_finalize(L);
}
