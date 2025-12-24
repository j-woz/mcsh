
# exit

function f { x y z:int=789 } {
  print f_result x $x y $y z $z
}

print ok
# exit

f 1 2 3
f 1 2
# TODO: handle this error:
f 1
