#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# We trust that the user has a recent enough autoconf & automake setup
# (not older than a few years...)
set -e

if test $(uname -s) = "OpenBSD" ; then
	# OpenBSD needs these environment variables set.
	AUTOCONF_VERSION=2.69
	AUTOMAKE_VERSION=1.11
	export AUTOCONF_VERSION
	export AUTOMAKE_VERSION
	fi

srcdir=`dirname $0`
test -n "$srcdir" && cd "$srcdir"

echo "Updating build configuration files for FLAC, please wait...."

touch config.rpath
autoreconf -isf
#./configure "$@" && echo
