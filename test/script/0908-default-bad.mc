
# Mixing NAME=VALUE pairs with other arguments is an error.
# TEST:FAIL
# TEST:EXPECT: give NAME VALUE, or one or more NAME=VALUE

default k1=v1 x 3
