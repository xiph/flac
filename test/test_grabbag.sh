#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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
