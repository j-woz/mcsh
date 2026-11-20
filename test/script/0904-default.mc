
# default is a synonym for := : assign only if not already set.
# TEST:EXPECT: x is 10
# TEST:EXPECT: still 10

default x 10
print x is $x
default x 20
print still $x
