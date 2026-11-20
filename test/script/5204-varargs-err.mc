
# This test would sum its integer arguments,
# but we provide x so it should result in an uncaught exception

# TEST:ARGS_SCRIPT: 1 2 x 4
# TEST:FAIL
# TEST:EXPECT: requires a int

signature A:int...
