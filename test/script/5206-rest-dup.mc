
# A rest parameter cannot repeat another parameter's name.
# TEST:FAIL
# TEST:EXPECT: rest parameter repeats a parameter name

function f { x x... } {
  print $x
}
