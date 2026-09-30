# abs() preserves the operand type: int stays int, float stays float
print (( $ abs( -7 ) ))
print (( $ abs( -2.5 ) ))
print (( $ abs( 4 ) ))
# TEST:EXPECT: 7
# TEST:EXPECT: 2.5
# TEST:EXPECT: 4
