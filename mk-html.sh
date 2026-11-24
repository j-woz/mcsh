#!/bin/sh
set -e

# Generate README.html from README.adoc.m4, for local viewing.
# Runs mk-doc.sh first so README.adoc is current, then renders it
# with asciidoctor.

if ! command -v asciidoctor > /dev/null
then
  echo "mk-html.sh: asciidoctor not found; install it or ask" >&2
  exit 1
fi

./mk-doc.sh
asciidoctor -o README.html README.adoc
echo "wrote README.html"
