
# TEST:SKIP

= P (( pattern { { $1 == "A" } { print "FOUND" } } ))
stream $P 9201-mck-1.data
