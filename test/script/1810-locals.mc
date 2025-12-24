
= x 1
print PX1 $x

function g { x } {
  = x 3
  = y 6
  print PX3 $x
  print PY6 $y
}


function f { x } {
  = x 2
  = y 5
  print PX2 $x
  print PY51 $y
  g data
  print PY52 $y
}

f data

print PX1 $x
