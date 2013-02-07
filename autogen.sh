#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# We trust that the user has a recent enough autoconf & automake setup
# (not older than a few years...)
set -e

srcdir=`dirname $0`
test -n "$srcdir" && cd "$srcdir"

echo "Updating build configuration files for FLAC, please wait...."

touch config.rpath
autoreconf -isf
#./configure "$@" && echo
