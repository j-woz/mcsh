
#include "strmap.h"

#include "inttypes.h"

bool
strmap_realloc_capacity(strmap* map)
{
  size_t new = map->capacity * 2;
  DEBUG(DBG, "strmap_realloc_capacity: %zi -> %zi",
             map->capacity, new);
  map->keys     = realloc_checked(map->keys, new * sizeof(char*));
  map->data     = realloc_checked(map->data, new * sizeof(void*));
  map->capacity = new;
  return true;
}

bool
strmap_realloc_keys(strmap* map)
{
  size_t length_new = map->length * 2;

  DEBUG(DBG, "strmap_realloc_keys: %zi -> %zi",
             map->length, length_new);

  ptrdiff_t n = map->tail - map->text;

  int16_t D[map->size];
  for (size_t i = 0; i < map->size; i++)
  {
    if (map->keys[i] != NULL)
      D[i] = map->keys[i] - map->text;
    else
      D[i] = -1;
  }

  map->length = length_new;
  map->text   = realloc_checked(map->text, map->length);
  map->tail   = map->text + n;

  for (size_t i = 0; i < map->size; i++)
    if (D[i] != -1)
      map->keys[i] = map->text + D[i];
    else
      map->keys[i] = NULL;

  DEBUG(DBG, "strmap_realloc_keys: OK");
  return true;
}

void
strmap_defrag(strmap* map)
{
  DEBUG(DBG, "defrag...");
  char* p = map->text;
  size_t j = 0;  // New index
  for (size_t i = 0; i < map->size; i++)
  {
    if (map->keys[i] == NULL) continue;
    size_t n = strlen(map->keys[i]);
    // Copy trailing NUL byte too
    memmove(p, map->keys[i], n+1);
    map->keys[j] = p;
    p += n+1;

    map->data[j] = map->data[i];
    j++;
  }
  map->tail = p;
  map->size     = j;
  map->drops    = 0;
  map->frags    = 0;
}

void
strmap_show(strmap* map)
{
  printf("strmap: [%p] %zi/%zi %zi drops=%zi frags=%zi\n",
         map, map->size, map->capacity, map->length,
         map->drops, map->frags);

  strmap_show_text(map);
  strmap_show_keys(map);
  strmap_show_data(map);
}

void
strmap_show_text(strmap* map)
{
  ptrdiff_t n = map->tail - map->text;
  if (n == 0)
  {
    printf("(empty)\n");
    return;
  }
  char* tmp = calloc(n, 1);
  memcpy(tmp, map->text, n);
  for (int i = 0; i < n; i++)
    if (tmp[i] == '\0') tmp[i] = '|';
  tmp[n-1] = '\0';
  printf("keys: [%zi] %s\n", n, tmp);
  free(tmp);
}

void
strmap_show_keys(strmap* map)
{
  char b[16];
  for (size_t i = 0; i < map->size; i++)
  {
    if (map->keys[i] == NULL)
      strcpy(b, "NULL");
    else
      sprintf(b, "%p", map->keys[i]);
    printf("%zi=%-14s ", i, b);
  }
  printf("\n");
}

void
strmap_show_data(strmap* map)
{
  char b[16];
  for (size_t i = 0; i < map->size; i++)
  {
    if (map->data[i] == NULL)
      strcpy(b, "NULL");
    else
      sprintf(b, "%p", map->data[i]);
    printf("%zi=%-14s ", i, b);
  }
}
