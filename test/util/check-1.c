
#include <util.h>

// CHECK_FAILED w/o args

int
main()
{
  CHECK_FAILED("check-1 failed.");
  return 0;
}
