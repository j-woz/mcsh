
# TEST:FAIL

# Syntax error: defaults must be denoted with = not :
# TEST:EXPECT: signature default must use '=', not ':'

function f { x y z:int:789 } {
  print f_result x $x y $y z $z
}
