#!/bin/sh
set -e

# Generate README.adoc from README.adoc.m4

if [ -f README.adoc ]
then
  chmod u+w README.adoc
fi
m4 -P < README.adoc.m4 > README.adoc
chmod u-w README.adoc
