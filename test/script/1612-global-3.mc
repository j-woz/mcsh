# assume x is set on the command line
# TEST:ARGS_MCSH: x=42 y=1
global x
global y
= z (( $ $x + $y ))
print $x
