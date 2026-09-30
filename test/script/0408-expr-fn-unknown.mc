# Calling a function that is neither a math builtin nor user-defined
# raises an exception
# TEST:FAIL
# TEST:EXPECT: unknown function in expr: 'bogus'
print (( $ bogus( 1 ) ))
