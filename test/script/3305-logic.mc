
# Logical AND
print (( $ 1 && 1 ))
# TEST:EXPECT: 1
print (( $ 1 && 0 ))
# TEST:EXPECT: 0

# Non-1 operands still yield boolean 0/1
print (( $ 2 && 3 ))
# TEST:EXPECT: 1

# Logical OR
print (( $ 0 || 0 ))
# TEST:EXPECT: 0
print (( $ 1 || 0 ))
# TEST:EXPECT: 1

# Comparisons bind tighter than &&
print (( $ 1 < 2 && 3 < 4 ))
# TEST:EXPECT: 1

# && binds tighter than ||
print (( $ 0 || 0 && 1 ))
# TEST:EXPECT: 0
