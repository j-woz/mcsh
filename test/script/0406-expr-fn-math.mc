# builtin math functions in expr syntax, including nesting and
# combination with operators
print (( $ sqrt( 16 ) ))
print (( $ pow( 2, 10 ) ))
print (( $ max( 3, 7 ) ))
print (( $ min( 3, 7 ) ))
print (( $ sqrt( abs( -16 ) ) ))
print (( $ abs( -2 ) + 10 ))
# TEST:EXPECT: 4.0
# TEST:EXPECT: 1024.0
# TEST:EXPECT: 7.0
# TEST:EXPECT: 3.0
# TEST:EXPECT: 12
