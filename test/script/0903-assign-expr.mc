
# Attached assignment of an expression: W=(( ... )).
# TEST:EXPECT: W is 5

W=(( $ 2 + 3 ))
print W is $W
