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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../obj/bin:$PATH
BINS_PATH=../../test_files/bins

test -d ${BINS_PATH} || exit 77

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name: encode..."
	cmd="flac --verify --silent --force-raw-input --endian=big --sample-rate=44100 --bps=$bps --channels=$channels $encode_options $name.bin"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	if $cmd 2>>./streams.log ; then : ; else
		echo "ERROR during encode of $name" 1>&2
		exit 1
	fi
	echo -n "decode..."
	cmd="flac --silent --endian=big --decode --force-raw-input $name.flac";
	echo "### DECODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	if $cmd 2>>./streams.log ; then : ; else
		echo "ERROR during decode of $name" 1>&2
		exit 1
	fi
	ls -1l $name.bin >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.raw >> ./streams.log
	echo -n "compare..."
	if cmp $name.bin $name.raw ; then : ; else
		echo "ERROR during compare of $name" 1>&2
		exit 1
	fi
	echo OK
}

echo "Testing bins..."
for f in b00 b01 b02 b03 ; do
	for opt in 0 1 2 4 5 6 8 ; do
		for extras in '' '--qlp-coeff-precision-search' '--exhaustive-model-search' ; do
			for blocksize in '' '--blocksize=32' '--blocksize=32768' '--blocksize=65535' ; do
				for channels in 1 2 4 8 ; do
					for bps in 8 16 24 ; do
						test_file $BINS_PATH/$f $channels $bps "-$opt $extras $blocksize"
					done
				done
			done
		done
	done
done
