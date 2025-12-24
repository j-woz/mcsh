
function f { ... } {
  print "args:" $args
  print "arg 0" $0
  # TEST:EXPECT: arg 1 3 a c
  print "arg 1 3" $1 $3
  print "arg @" $@
  # TEST:EXPECT: arg @ [a,b,c]
  print "arg count" $*
  # TEST:EXPECT: arg count 3
}

f a b c
