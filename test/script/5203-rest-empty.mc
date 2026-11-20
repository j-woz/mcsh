
# A rest parameter with nothing left over is an empty list.
# TEST:EXPECT: rest: [] count: 0

function f { x rest... } {
  print rest: $rest count: $#rest
}

f a
