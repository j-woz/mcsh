
# $@, $* and $# report only the positional arguments: not flags,
# flag values, or name=value arguments.
# TEST:EXPECT: all: [a] count: 1 first: a
# TEST:EXPECT: all: [p,q] count: 2 first: p

function f { -v:bool=false -o=out x y=_ } {
  print all: $@ count: $# first: $1
}

f a -v y=b
f -o file p q
