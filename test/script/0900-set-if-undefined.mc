# Test := operator (set if undefined)
# Should set variable if not defined
:= x 10
print "TEST_RESULT:" $x

# Should not overwrite if already defined
:= x 20
print "TEST_RESULT:" $x

# Local Variables:
# mode: sh
# End:
