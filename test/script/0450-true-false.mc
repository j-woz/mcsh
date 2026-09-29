
# Boolean literal: true
print (( $ true ))
# TEST:EXPECT: 1

# Boolean literal: false
print (( $ false ))
# TEST:EXPECT: 0

# true in arithmetic
print (( $ true + 1 ))
# TEST:EXPECT: 2

# false in arithmetic
print (( $ false + 5 ))
# TEST:EXPECT: 5

# true in comparison
print (( $ true == 1 ))
# TEST:EXPECT: 1

# false in comparison
print (( $ false == 0 ))
# TEST:EXPECT: 1

# true with logical AND
print (( $ true && true ))
# TEST:EXPECT: 1

# false with logical OR
print (( $ false || true ))
# TEST:EXPECT: 1

# Case insensitivity: True
print (( $ True ))
# TEST:EXPECT: 1

# Case insensitivity: FALSE
print (( $ FALSE ))
# TEST:EXPECT: 0
