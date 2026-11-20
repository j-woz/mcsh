
# A global set on the command line counts as set for default, and
# can be assigned with = without a crash.
# TEST:ARGS_MCSH: g=5
# TEST:EXPECT: g is 5
# TEST:EXPECT: g now 7

default g 3
print g is $g
= g 7
print g now $g
