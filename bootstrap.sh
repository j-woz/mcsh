#!/bin/sh
set -ex

# BOOTSTRAP SH
# Simply run autoreconf

autoreconf -Wno-portability --install
