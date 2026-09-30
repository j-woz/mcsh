# A math builtin called with the wrong number of arguments raises
# TEST:FAIL
# TEST:EXPECT: requires 1 argument
print (( $ sqrt( 1, 2 ) ))
