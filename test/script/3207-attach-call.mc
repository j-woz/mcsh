
# Attached assignment of a user function call: V=(( f $x )).
# TEST:EXPECT: V 4

function f { x } {
  $ $x + 1
}
= x 3
V=(( f $x ))
print V $V
