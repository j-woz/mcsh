
#include <util.h>

// CHECK_FAILED w/o args

int
main()
{
  CHECK(false, "check-3 failed.");
  return 0;
}
