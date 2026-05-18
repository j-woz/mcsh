
# TEST:FAIL

function f { x y z:int:789 } {
  print f_result x $x y $y z $z
}

print ok
# exit

f 1 2 3
f 1 2

# Results in error (properly an exception, good):
f 1
