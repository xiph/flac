#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2004,2005  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

die ()
{
	echo $* 1>&2
	exit 1
}

if [ x = x"$1" ] ; then 
	BUILD=debug
else
	BUILD="$1"
fi

LD_LIBRARY_PATH=../src/libFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/libOggFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../obj/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:$PATH
PATH=../src/metaflac:$PATH
PATH=../src/test_seeking:$PATH
PATH=../src/test_streams:$PATH
PATH=../obj/$BUILD/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"
metaflac --help 1>/dev/null 2>/dev/null || die "ERROR can't find metaflac executable"

run_flac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 flac $* 4>>test_seeking.valgrind.log
	else
		flac $*
	fi
}

run_metaflac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 metaflac $* 4>>test_seeking.valgrind.log
	else
		metaflac $*
	fi
}

run_test_seeking ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 test_seeking $* 4>>test_seeking.valgrind.log
	else
		test_seeking $*
	fi
}


echo "Generating streams..."
if [ ! -f noise.raw ] ; then
	test_streams || die "ERROR during test_streams"
fi

echo "generating FLAC files for seeking:"
run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=8 --channels=1 --blocksize=576 --output-name=tiny.flac noise8m32.raw || die "ERROR generating FLAC file"
run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 --blocksize=576 --output-name=small.flac noise.raw || die "ERROR generating FLAC file"
echo "generating Ogg FLAC files for seeking:"
run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=8 --channels=1 --blocksize=576 --output-name=tiny.ogg --ogg noise8m32.raw || die "ERROR generating Ogg FLAC file"
run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 --blocksize=576 --output-name=small.ogg --ogg noise.raw || die "ERROR generating Ogg FLAC file"

echo "testing tiny.flac:"
if run_test_seeking tiny.flac 100 ; then : ; else
	die "ERROR: during test_seeking"
fi

echo "testing small.flac:"
if run_test_seeking small.flac 1000 ; then : ; else
	die "ERROR: during test_seeking"
fi

echo "removing sample count from tiny.flac and small.flac:"
if run_metaflac --no-filename --set-total-samples=0 tiny.flac small.flac ; then : ; else
	die "ERROR: during metaflac"
fi

echo "testing tiny.flac with total_samples=0:"
if run_test_seeking tiny.flac 100 ; then : ; else
	die "ERROR: during test_seeking"
fi

echo "testing small.flac with total_samples=0:"
if run_test_seeking small.flac 1000 ; then : ; else
	die "ERROR: during test_seeking"
fi

echo "testing tiny.ogg:"
if run_test_seeking tiny.ogg 100 ; then : ; else
	die "ERROR: during test_seeking"
fi

echo "testing small.ogg:"
if run_test_seeking small.ogg 1000 ; then : ; else
	die "ERROR: during test_seeking"
fi

rm tiny.flac tiny.ogg small.flac small.ogg
