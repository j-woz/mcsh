
# All equal -> 1
print (( string = a a a ))
# TEST:EXPECT: 1

# Not all equal -> 0
print (( string = a b a ))
# TEST:EXPECT: 0

# Two equal -> 1
print (( string = hi hi ))
# TEST:EXPECT: 1
