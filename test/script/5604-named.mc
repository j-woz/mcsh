
# TEST :FAIL

function f { x:int } {
  print f_result x $x
}

 #f 42

# Should trigger error:
f s
