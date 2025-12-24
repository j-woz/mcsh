
fib()
{
  local n=$1
  if (( n == 0 ))
  then
    REPLY=0
    return
  elif (( n == 1 ))
  then
    REPLY=1
    return
  fi

  fib $(( $n - 1 ))
  local f1=$REPLY
  fib $(( $n - 2 ))
  local f2=$REPLY
  REPLY=$(( f1 + f2 ))
}

n=$1
fib $n
echo FIB $n "->" $REPLY
