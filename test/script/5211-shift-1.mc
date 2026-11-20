
# TEST:ARGS_SCRIPT: x y z

# TEST:EXPECT: arg1: x
print arg1: $1
shift
# TEST:EXPECT: arg2: y
print arg2: $1
