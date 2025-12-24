
= p1 (( bg sleep 3 ))
= p2 (( bg sleep 5 ))
print PIDs: $p1 $p2
print JOBS:
jobs
flush
print WAIT...
wait $p1
print DONE: $p1
flush
wait $p2
print DONE: $p2
flush
