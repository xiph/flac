#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002  Josh Coalson
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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../src/share/grabbag/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/test_grabbag/cuesheet:../obj/release/b:../obj/debug/bin:$PATH

test_cuesheet -h 1>/dev/null 2>/dev/null || (echo "ERROR can't find test_cuesheet executable" 1>&2 && exit 1)
if [ $? != 0 ] ; then exit 1 ; fi

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
	test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		echo "Error: test script is broken"
		exit 1
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
	test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		echo "Error: test script is broken"
		exit 1
	elif [ "$exit_code" != 0 ] ; then
		echo "Error: good cuesheet is broken"
		exit 1
	fi
	cuesheet_pass1=${cuesheet}.1
	cuesheet_pass2=${cuesheet}.2
	diff $cuesheet_pass1 $cuesheet_pass2 >> $log 2>&1
	if [ $? != 0 ] ; then
		echo "Error: pass1 and pass2 output differ"
		exit 1
	fi
	rm -f $cuesheet_pass1 $cuesheet_pass2
done

diff cuesheet.ok $log > cuesheet.diff
if [ $? != 0 ] ; then
	echo "Error: .log file does not match .ok file, see cuesheet.diff"
	exit 1
fi
