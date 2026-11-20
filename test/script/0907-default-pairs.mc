
# default (and :=) take one or more NAME=VALUE pairs.
# TEST:EXPECT: k1: v1 k2: v2 k3: v3 k4: v4

default k1=v1 k2=v2
default k1=other k3=v3
:= k4=v4
print k1: $k1 k2: $k2 k3: $k3 k4: $k4
