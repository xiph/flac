#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003  Josh Coalson
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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../src/share/grabbag/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/test_grabbag/cuesheet:../obj/release/bin:../obj/debug/bin:$PATH

test_cuesheet -h 1>/dev/null 2>/dev/null || die "ERROR can't find test_cuesheet executable"

run_test_cuesheet ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 test_cuesheet $* 4>>test_grabbag.valgrind.log
	else
		test_cuesheet $*
	fi
}

########################################################################
#
# test_cuesheet
#
########################################################################

log=cuesheet.log
bad_cuesheets=cuesheets/bad.*.cue
good_cuesheets=cuesheets/good.*.cue
good_leadout=`expr 80 \* 60 \* 44100`
bad_leadout=`expr $good_leadout + 1`

rm -f $log

#
# negative tests
#
for cuesheet in $bad_cuesheets ; do
	echo "NEGATIVE $cuesheet" >> $log 2>&1
	run_test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		die "Error: test script is broken"
	fi
	cuesheet_pass1=${cuesheet}.1
	cuesheet_pass2=${cuesheet}.2
	rm -f $cuesheet_pass1 $cuesheet_pass2
done

#
# positve tests
#
for cuesheet in $good_cuesheets ; do
	echo "POSITIVE $cuesheet" >> $log 2>&1
	run_test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		die "Error: test script is broken"
	elif [ "$exit_code" != 0 ] ; then
		die "Error: good cuesheet is broken"
	fi
	cuesheet_pass1=${cuesheet}.1
	cuesheet_pass2=${cuesheet}.2
	diff $cuesheet_pass1 $cuesheet_pass2 >> $log 2>&1 || die "Error: pass1 and pass2 output differ"
	rm -f $cuesheet_pass1 $cuesheet_pass2
done

diff cuesheet.ok $log > cuesheet.diff || die "Error: .log file does not match .ok file, see cuesheet.diff"

echo "PASSED (results are in $log)"
