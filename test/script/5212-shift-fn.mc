
# shift in a function, and in an inplace call, which shares its
# caller's arguments.  $+1 tests for an argument without raising.
# TEST:EXPECT: start: a count: 4
# TEST:EXPECT: after shift: b
# TEST:EXPECT: after inplace shift 2: d name: f
# TEST:EXPECT: left: d
# TEST:EXPECT: done count: 0

inplace drop2 { } {
  shift 2
}

function f { args... } {
  print start: $1 count: $#
  shift
  print after shift: $1
  drop2
  print after inplace shift 2: $1 name: $0
  loop while { $ $+1 } {
    print left: $1
    shift
  }
  print done count: $#
}

f a b c d
