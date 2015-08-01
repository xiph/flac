#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# We trust that the user has a recent enough autoconf & automake setup
# (not older than a few years...)

use_symlinks=" --symlink"

case $1 in
	--no-symlink*)
		use_symlinks=""
		echo "Copying autotool files instead of using symlinks."
		;;
	*)
		echo "Using symlinks to autotool files (use --no-symlinks to copy instead)."
		;;
	esac

test_program_errors=0

test_program () {
	if ! command -v $1 >/dev/null 2>&1 ; then
		echo "Missing program '$1'."
		test_program_errors=1
		fi
}

for prog in autoconf automake libtool pkg-config ; do
	test_program $prog
	done

if test $(uname -s) != "Darwin" ; then
	test_program gettext
	fi

test $test_program_errors -ne 1 || exit 1

#-------------------------------------------------------------------------------

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
autoreconf --install $use_symlinks --force
#./configure "$@" && echo
