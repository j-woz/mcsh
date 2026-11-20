
# default leaves an environment variable alone: it counts as set.
# TEST:EXPECT: same: 0

default PATH x
print same: (( string = $PATH x ))
