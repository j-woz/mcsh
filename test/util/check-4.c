
#include <util.h>

// CHECK(false) w/o args

int
main()
{
  CHECK(false, "check-4 error. value: '%s' was %i",
               "x", 42);
  return 0;
}
