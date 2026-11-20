
# signature binds a rest parameter from the command line.  After
# signature, $@, $* and $# see only the positional arguments.

# TEST:ARGS_SCRIPT: a -v b c
# TEST:EXPECT: x: a v: true rest: [b,c]
# TEST:EXPECT: all: [a,b,c] count: 3
# TEST:EXPECT: words: a b c

signature -v:bool=false x rest...
print x: $x v: $v rest: $rest
print all: $@ count: $#
print words: $*
