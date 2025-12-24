
#include <util.h>

// CHECK_FAILED w/ args

int
main()
{
  CHECK_FAILED("check-2 failed. value: '%s' was %i",
               "x", 42);
  return 0;
}
