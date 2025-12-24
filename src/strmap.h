
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

// Set to true to enable debugging
#define DBG false

typedef struct
{
  // Current number of entries, including dropped
  size_t size;
  // Capacity for entries: may be realloced:
  size_t capacity;
  // Length of key text: may be realloced:
  size_t length;
  // Flat storage for all keys:
  char*  text;
  // Pointer to end of used text
  char*  tail;
  // Pointers to each key in text:
  char** keys;
  // Pointers to the data; parallel with keys
  void** data;
  // Number of dropped entries:
  size_t drops;
  // Wasted space created by dropped entries:
  size_t frags;
} strmap;

static inline bool
strmap_init(strmap* map, int capacity)
{
  map->size = 0;
  map->capacity = capacity;
  map->length   = capacity * 4;
  map->text     = malloc_checked(sizeof(char*) * map->length);
  map->tail     = map->text;
  map->keys     = malloc_checked(capacity * sizeof(char*));
  map->data     = malloc_checked(capacity * sizeof(void*));
  map->drops    = 0;
  map->frags    = 0;
  return true;
}

void strmap_show(strmap* map);
void strmap_show_text(strmap* map);
void strmap_show_keys(strmap* map);
void strmap_show_data(strmap* map);

bool strmap_realloc_capacity(strmap* map);
bool strmap_realloc_keys(strmap* map);

/** Add to the strmap.  Copies the key, points to the data. */
static inline bool
strmap_add(strmap* map, const char* key, void* data)
{
  DEBUG(DBG, "strmap_add: '%s' @ %zi", key, map->size);
  if (map->size == map->capacity)
    strmap_realloc_capacity(map);
  size_t current = map->tail - map->text;
  size_t key_space = strlen(key) + 2;
  while (current + key_space + 1 > map->length)
    strmap_realloc_keys(map);
  map->keys[map->size] = map->tail;
  char* end = stpncpy(map->tail, key, key_space);
  map->tail = end + 1;
  map->data[map->size] = data;
  map->size++;
  return true;
}

static inline bool
strmap_search(strmap* map, const char* key, void** data)
{
  DEBUG(DBG, "search: '%s'", key);
  size_t k = strlen(key);
  for (size_t i = 0; i < map->size; i++)
  {
    char* t = map->keys[i];
    if (t == NULL) continue;
    size_t n = strlen(t);
    if (k == n && strncmp(key, t, n) == 0)
    {
      if (data != NULL) *data = map->data[i];
      return true;
    }
  }
  // Not found:
  return false;
}

static inline bool
strmap_search_index(strmap* map, const char* key, size_t* index)
{
  size_t k = strlen(key);
  for (size_t i = 0; i < map->size; i++)
  {
    char* t = map->keys[i];
    if (t == NULL) continue;
    size_t n = strlen(t);
    if (k == n && strncmp(key, t, n) == 0)
    {
      *index = i;
      return true;
    }
  }
  // Not found:
  return false;
}

/** Note that indices update after a drop+defrag! */
static inline char*
strmap_get_key(strmap* map, size_t index)
{
  return map->keys[index];
}

/** Note that indices update after a drop+defrag! */
static inline void*
strmap_get_value(strmap* map, size_t index)
{
  return map->data[index];
}

void strmap_defrag(strmap* map);

static inline void
strmap_drop_index(strmap* map, size_t index)
{
  valgrind_assert(index < map->size);
  DEBUG(DBG, "drop: %zi", index);
  valgrind_assert(map->keys[index] != NULL);
  DEBUG(DBG, "drop: %zi '%s'", index, map->keys[index]);
  size_t g = strlen(map->keys[index]);
  memset(map->keys[index], '\0', g);
  map->keys[index] = NULL;
  map->data[index] = NULL;
  map->drops ++;
  map->frags += g;
  if (map->frags > map->length / 2)
  {
    strmap_defrag(map);
  }
}

static inline void
strmap_set_value(strmap* map, size_t index, void* data)
{
  map->data[index] = data;
}

static inline void
strmap_finalize(strmap* map)
{
  free(map->text);
  free(map->keys);
  free(map->data);
}

static inline void
strmap_free(strmap* map)
{
  strmap_finalize(map);
  free(map);
}
