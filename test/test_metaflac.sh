#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2002  Josh Coalson
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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../src/metaflac:../obj/bin:$PATH

flacfile=metaflac.flac

flac --help 1>/dev/null 2>/dev/null || echo "ERROR can't find flac executable" 1>&2 && exit 1
metaflac --help 1>/dev/null 2>/dev/null || echo "ERROR can't find metaflac executable" 1>&2 && exit 1

echo "Generating streams..."
if flac -V -0 -o $flacfile -fr -fb -fc 1 -fp 8 -fs 44100 `which metaflac` ; then : ; else
	echo "ERROR during generation" 1>&2
	exit 1
fi

check_exit ()
{
	exit_code=$?
	if [ $exit_code != 0 ] ; then
		echo "ERROR, exit code = $exit_code" 1>&2
		exit 1
	fi
}

check_flac ()
{
	if flac -t $flacfile ; then : ; else
		echo "ERROR if $flacfile" 1>&2
		exit 1
	fi
}

check_flac
metaflac --list $flacfile
