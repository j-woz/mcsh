
# Command-line text is converted to the declared type: 42 -> int
# TEST:ARGS_SCRIPT: 42 43 44
# TEST:EXPECT: 44
signature x:int y z
print $z
