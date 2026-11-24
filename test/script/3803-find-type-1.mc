# TEST:FAIL
# A bare number is an int, not a string: find must reject it, not crash
print (( find 0123456789 "2" ))
