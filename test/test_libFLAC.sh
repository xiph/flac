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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/test_libFLAC:../obj/release/bin:../obj/debug/bin:$PATH
export PATH

run_test_libFLAC ()
{
	if [ "$FLAC__VALGRIND" = yes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 test_libFLAC $* 4>>test_libFLAC.valgrind.log
	else
		test_libFLAC $*
	fi
}

run_test_libFLAC || die "ERROR during test_libFLAC"
