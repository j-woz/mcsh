
function f { a... } {
  # TEST:EXPECT: a: [A,B,C]
  print "a:" $a
  # TEST:EXPECT: arg 0 f
  print "arg 0" $0
  # TEST:EXPECT: arg 1 3 A C
  print "arg 1 3" $1 $3
  print "arg @" $@
  # TEST:EXPECT: arg @ [A,B,C]
  print "args" $*
  # TEST:EXPECT: args A B C
}

f A B C
