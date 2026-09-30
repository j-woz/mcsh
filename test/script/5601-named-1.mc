
# TEST:FAIL

# Positional, default, and named-argument calls against a signature with
# a typed default in the last slot (z:int=789).
# TEST:EXPECT: f_result x 1 y 2 z 3
# TEST:EXPECT: f_result x 1 y 2 z 789
# TEST:EXPECT: f_result x 1 y 2 z 55
# TEST:EXPECT: f_result x 10 y 20 z 30
# TEST:EXPECT: f_result x 1 y 9 z 5
# TEST:EXPECT: did not assign to: 'y'

function f { x y z:int=789 } {
  print f_result x $x y $y z $z
}

print ok
# exit

f 1 2 3
f 1 2

# Named argument overrides the default:
f 1 2 z=55
# All arguments named:
f x=10 y=20 z=30
# Positional x, then named z and y out of order:
f 1 z=5 y=9

# Results in error (properly an exception, good):
f 1
