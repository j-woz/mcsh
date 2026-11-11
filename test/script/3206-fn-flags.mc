
# The same flag grammar drives function calls, not just command-line args:
# -v is a bool flag (presence sets true), -o a value flag, file positional.
# TEST:EXPECT: run v false o out file in.txt
# TEST:EXPECT: run v true o out file in.txt
# TEST:EXPECT: run v true o result.txt file in.txt

function run { -v:bool=false -o=out file } {
  print run v $v o $o file $file
}

# No flags: defaults apply.
run in.txt
# Bool flag present.
run -v in.txt
# Bool flag plus a value flag, in any order before the positional.
run -v -o result.txt in.txt
