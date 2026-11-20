
# The README examples for positional arguments and shift.
# TEST:EXPECT: first: a count: 3
# TEST:EXPECT: list: [a,b,c]
# TEST:EXPECT: words: a b c
# TEST:EXPECT: item: x
# TEST:EXPECT: item: y
# TEST:EXPECT: item: z

function f { -v:bool=false x args... } {
  print first: $1 count: $#
  print list: $@
  print words: $*
}
f a -v b c

function each { args... } {
  loop while { $ $+1 } {
    print item: $1
    shift
  }
}
each x y z
