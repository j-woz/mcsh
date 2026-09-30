
# Named-argument call syntax: 'name=value' parses into a TAG (name=value
# pair) and is matched to the parameter by name, so order is free and
# named may be mixed with positional.
# TEST:EXPECT: f_result x s y t
# TEST:EXPECT: f_result x 1 y 2
# TEST:EXPECT: f_result x 7 y 8

function f { x y } {
  print f_result x $x y $y
}

# Both named, given in reversed order:
f y=t x=s
# Both named, natural order:
f x=1 y=2
# Positional x, named y:
f 7 y=8
