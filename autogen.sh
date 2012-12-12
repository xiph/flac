#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# We trust that the user has a recent enough autoconf & automake setup
# (not older than a few years...)
autoreconf -i
#$srcdir/configure "$@" && echo
