
function f { x y z ... } {
  print "xyz: " $x $y $z
  print "args1:" $args
  print "argc1:" $#args
  print "argc2:" $*
  print "args2:" $@
}

f a b c d e f
