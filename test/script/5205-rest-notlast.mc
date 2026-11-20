
# A rest parameter must be the last parameter.
# TEST:FAIL
# TEST:EXPECT: rest parameter must be the last parameter

function f { a... b } {
  print $a $b
}
