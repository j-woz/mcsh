
#include <stdio.h>
#include <unistd.h>

#include <strmap.h>

int
main()
{
  strmap map;
  strmap_init(&map, 2);

  int v101 = 101;
  int v102 = 102;
  int v103 = 103;

  strmap_add(&map, "hello1", &v101);
  strmap_add(&map, "hello2", &v102);
  strmap_add(&map, "hello3", &v103);

  printf("size: %zi\n", map.size);
  for (int i = 0; i < map.size; i++)
  {
    printf("entry: %i %s\n", i, map.keys[i]);
  }

  strmap_show_text(&map);

  int* t = NULL;
  strmap_search(&map, "hello2", (void*) &t);
  printf("found: %i\n", *t);

  strmap_finalize(&map);

  return EXIT_SUCCESS;
}
