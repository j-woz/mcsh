
# Too many arguments for a function without a rest parameter.
# TEST:FAIL
# TEST:EXPECT: too many arguments: unexpected 'c'

function f { x y } {
  print $x $y
}

f a b c
