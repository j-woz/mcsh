
#include <stdio.h>
#include <unistd.h>

#include <strmap.h>

int
main()
{
  setbuf(stdout, NULL);

  strmap map;
  strmap_init(&map, 2);

  int count = 15;
  int L[count];
  char key[32];

  for (int i = 0; i < count; i++)
  {
    sprintf(key, "hellokey%05i", i);
    L[i] = i+100;
    strmap_add(&map, key, &L[i]);
  }

  printf("size: %zi\n", map.size);
  for (size_t i = 0; i < map.size; i++)
  {
    printf("entry: %zi %s\n", i, map.keys[i]);
  }

  strmap_show(&map);

  int* t = NULL;
  bool b = strmap_search(&map, "hello2", (void*) &t);
  printf("found: %i\n", b);

  strmap_drop_index(&map, 1);
  strmap_show(&map);

  strmap_finalize(&map);
  return EXIT_SUCCESS;
}
