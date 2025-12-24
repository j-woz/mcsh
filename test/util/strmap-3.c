
#include <stdio.h>
#include <unistd.h>

#include <strmap.h>

int
main()
{
  setbuf(stdout, NULL);

  strmap map;
  strmap_init(&map, 2);

  int v101 = 101;
  int v102 = 102;
  int v103 = 103;
  int v104 = 104;

  strmap_add(&map, "hello1", &v101);
  strmap_add(&map, "hello2", &v102);
  strmap_add(&map, "hello3", &v103);

  printf("size: %zi\n", map.size);
  for (size_t i = 0; i < map.size; i++)
  {
    printf("entry: %zi %s\n", i, map.keys[i]);
  }

  strmap_show(&map);

  int* t = NULL;
  strmap_search(&map, "hello2", (void*) &t);
  printf("found: %i\n", *t);

  strmap_drop_index(&map, 1);
  strmap_show(&map);

  bool rc;
  rc = strmap_search(&map, "hello2", (void*) &t);
  printf("found 1: %i\n", rc);

  char k[64];
  int* N[128];
  for (int i = 4; i < 8; i++)
  {
    printf("\n");
    sprintf(k, "hello%i", i);
    strmap_add(&map, k, &v104);
    strmap_show(&map);
    printf("\n");
    rc = strmap_search(&map, k, (void*) &N[i]);
    printf("found %i: %i %i\n", i, rc, *N[i]);
    strmap_show(&map);
    printf("\n");
    if (i % 2 == 0)
    {
      strmap_drop_index(&map, i-1);
      strmap_show(&map);
      printf("\n");
    }
  }

  strmap_drop_index(&map, 2);
  strmap_show(&map);
  printf("\n");

  strmap_drop_index(&map, 4);
  strmap_show(&map);
  printf("\n");


  strmap_drop_index(&map, 0);
  strmap_show(&map);
  printf("\n");

  strmap_finalize(&map);
  return EXIT_SUCCESS;
}
