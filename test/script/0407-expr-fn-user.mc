# A name not in the math table falls back to a user-defined function
function inc { x } {
  return (( $ $x + 1 ))
}
function add { a b } {
  return (( $ $a + $b ))
}
print (( $ inc( 41 ) ))
print (( $ add( 20, 22 ) ))
# TEST:EXPECT: 42
