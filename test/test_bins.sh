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

die ()
{
	echo $* 1>&2
	exit 1
}

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../obj/release/bin:../obj/debug/bin:$PATH
BINS_PATH=../../test_files/bins

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ "$FLAC__VALGRIND" = yes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=10 --logfile-fd=4 flac $* 4>>valgrind.log
	else
		flac $*
	fi
}

test -d ${BINS_PATH} || exit 77

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name (--channels=$channels --bps=$bps $encode_options): encode..."
	cmd="run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $encode_options $name.bin"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during encode of $name"

	echo -n "decode..."
	cmd="run_flac --silent --endian=big --sign=signed --decode --force-raw-format $name.flac";
	echo "### DECODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during decode of $name"

	ls -1l $name.bin >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.raw >> ./streams.log

	echo -n "compare..."
	cmp $name.bin $name.raw || die "ERROR during compare of $name"

	echo OK
}

echo "Testing bins..."
for f in b00 b01 b02 b03 b04 ; do
	binfile=$BINS_PATH/$f
	if [ -f $binfile ] ; then
		for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
			for channels in 1 2 4 8 ; do
				for bps in 8 16 24 ; do
					for opt in 0 1 2 4 5 6 8 ; do
						for extras in '' '-p' '-e' ; do
							for blocksize in '' '--lax -b 32' '--lax -b 32768' '--lax -b 65535' ; do
								test_file $binfile $channels $bps "-$opt $extras $blocksize $disable"
							done
						done
					done
					if [ "$FLAC__EXHAUSTIVE_TESTS" = yes ] ; then
						test_file $binfile $channels $bps "-b 16384 -m -r 8 -l 32 -e -p $disable"
					fi
				done
			done
		done
	else
		echo "$binfile not found, skipping."
	fi
done
