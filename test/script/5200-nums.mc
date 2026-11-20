
# args is a user-provided symbol

function f { x y z args... } {
  print "xyz: " $x $y $z
  # TEST:EXPECT: args: [d,e,f]
  print "args:" $args
  # TEST:EXPECT: argc 1: 3
  print "argc 1:" $#args
  # $* Should expand to multiple arguments to the print builtin
  #    via list expansion
  # TEST:EXPECT: arg strings 2: a b c d e f
  print "arg strings 2:" $*
  # $@ should expand to a single list containing all the arguments
  # TEST:EXPECT: [a,b,c,d,e,f]
  print "arg list 3:" $@
}

f a b c d e f
