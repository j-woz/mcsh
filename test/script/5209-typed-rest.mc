
# A typed rest parameter type-checks each argument.  In a call, an
# int converts to a float parameter, and 0 or 1 to a bool parameter.
# TEST:EXPECT: ns: [1,2,3] count: 3
# TEST:EXPECT: x: 3.0 b: true

function sum { ns:int... } {
  print ns: $ns count: $#ns
}
sum 1 2 3

function g { x:float b:bool } {
  print x: $x b: $b
}
g 3 1
