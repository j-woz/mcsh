
# TEST:FAIL

# Named-argument error: a name that matches no parameter in the
# signature raises an exception.
# TEST:EXPECT: f_result x 1 y 2 z 3
# TEST:EXPECT: no such argument: 'w'

function f { x y z:int=789 } {
  print f_result x $x y $y z $z
}

# A valid named call first:
f x=1 y=2 z=3

# 'w' is not a parameter of f: raises an exception.
f x=1 y=2 w=3
