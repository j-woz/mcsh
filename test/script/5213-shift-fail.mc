
# shift N with fewer than N arguments left drops them all and
# returns false.  argc and argv do not change.
# TEST:ARGS_SCRIPT: x y
# TEST:EXPECT: shift 1: true left: [y]
# TEST:EXPECT: shift 5: false left: []
# TEST:EXPECT: argc still: 3

= ok (( shift ))
print shift 1: $ok left: $@
= ok (( shift 5 ))
print shift 5: $ok left: $@
print argc still: $mcsh.argc
