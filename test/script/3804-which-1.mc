# TEST:EXPECT: f: function
# TEST:EXPECT: i: inplace
# TEST:EXPECT: m: macro
# TEST:EXPECT: print: builtin
# TEST:EXPECT: foreach: keyword
# TEST:EXPECT: nosuchthing_xyz: not found
function f { } { print f }
inplace i { } { print i }
macro m { } { print m }
which f i m print foreach nosuchthing_xyz
