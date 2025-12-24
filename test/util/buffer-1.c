
#include "buffer.h"

int
main()
{
  buffer B;
  buffer_init(&B, 8);

  buffer_cat(&B, "hello");
  buffer_show(&B);

  buffer_cat(&B, "x");
  buffer_show(&B);

  buffer_cat(&B, "bye");
  buffer_show(&B);

  buffer C;
  buffer_init(&C, 8);
  buffer_reset(&B);
  buffer_cat(&B, "jkl");
  buffer_cat(&C, " mno");
  buffer_catb(&B, &C);

  buffer_show(&B);
  buffer_show(&C);

  buffer_finalize(&B);
  buffer_finalize(&C);

  return 0;
}
