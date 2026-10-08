
# TEST:SKIP
# TEST:ARGS_SCRIPT: -f -x dog
signature -f:bool=false -g:bool=false -x=_ -y=_ z=data

# f should be true
# g should be false because flag is not given
# x should be "dog"
# y should be "_" (default, flag not given)
# z should be "data" because positional argument is not given

print $f $g $x $y $z
