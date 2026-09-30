
# TEST:FAIL

# Named arguments still enforce declared types.  Positional and named
# calls with correct types succeed; a named argument of the wrong type
# raises the same exception as the positional form.
# TEST:EXPECT: f_result x 1 y 2
# TEST:EXPECT: f_result x 3 y 4
# TEST:EXPECT: f_result x 5 y 6
# TEST:EXPECT: requires a int

function f { x:int y:int } {
  print f_result x $x y $y
}

f 1 2
f x=3 y=4
f y=6 x=5

# Should trigger error: x is given a string:
f x=s y=1
