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
PATH=../src/flac:../src/test_streams:../obj/release/bin:../obj/debug/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ "$FLAC__VALGRIND" = yes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=10 --logfile-fd=4 flac $* 4>>valgrind.log
	else
		flac $*
	fi
}

echo "Generating streams..."
if [ ! -f wacky1.wav ] ; then
	test_streams || die "ERROR during test_streams"
fi

#
# single-file test routines
#

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name (--channels=$channels --bps=$bps $encode_options): encode..."
	cmd="run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $encode_options $name.raw"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during encode of $name"

	echo -n "decode..."
	cmd="run_flac --silent --endian=big --sign=signed --decode --force-raw-format --output-name=$name.cmp $name.flac"
	echo "### DECODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during decode of $name"

	ls -1l $name.raw >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.cmp >> ./streams.log

	echo -n "compare..."
	cmp $name.raw $name.cmp || die "ERROR during compare of $name"

	echo OK
}

test_file_piped ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	if [ `env | grep -ic '^comspec='` != 0 ] ; then
		is_win=yes
	else
		is_win=no
	fi

	echo -n "$name: encode via pipes..."
	if [ $is_win = yes ] ; then
		cmd="run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $encode_options --stdout $name.raw"
		echo "### ENCODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		$cmd 1>$name.flac 2>>./streams.log || die "ERROR during encode of $name"
	else
		cmd="run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $encode_options --stdout -"
		echo "### ENCODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		cat $name.raw | $cmd 1>$name.flac 2>>./streams.log || die "ERROR during encode of $name"
	fi
	echo -n "decode via pipes..."
	if [ $is_win = yes ] ; then
		cmd="run_flac --silent --endian=big --sign=signed --decode --force-raw-format --stdout $name.flac"
		echo "### DECODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		$cmd 1>$name.cmp 2>>./streams.log || die "ERROR during decode of $name"
	else
		cmd="run_flac --silent --endian=big --sign=signed --decode --force-raw-format --stdout -"
		echo "### DECODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		cat $name.flac | $cmd 1>$name.cmp 2>>./streams.log || die "ERROR during decode of $name"
	fi
	ls -1l $name.raw >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.cmp >> ./streams.log

	echo -n "compare..."
	cmp $name.raw $name.cmp || die "ERROR during compare of $name"

	echo OK
}

if [ "$FLAC__EXHAUSTIVE_TESTS" = yes ] ; then
	max_lpc_order=32
else
	max_lpc_order=16
fi

echo "Testing noise through pipes..."
test_file_piped noise 1 8 "-0"

echo "Testing small files..."
test_file test01 1 16 "-0 -l $max_lpc_order -m -e -p"
test_file test02 2 16 "-0 -l $max_lpc_order -m -e -p"
test_file test03 1 16 "-0 -l $max_lpc_order -m -e -p"
test_file test04 2 16 "-0 -l $max_lpc_order -m -e -p"

echo "Testing 8-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd8-$b 1 8 "-0 -l $max_lpc_order -m -e -p"
done

echo "Testing 16-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd16-$b 1 16 "-0 -l $max_lpc_order -m -e -p"
done

echo "Testing 24-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd24-$b 1 24 "-0 -l $max_lpc_order -m -e -p"
done

echo "Testing 16-bit wasted-bits-per-sample streams..."
for b in 01 ; do
	test_file wbps16-$b 1 16 "-0 -l $max_lpc_order -m -e -p"
done

for bps in 8 16 24 ; do
	echo "Testing $bps-bit sine wave streams..."
	for b in 00 ; do
		test_file sine${bps}-$b 1 $bps "-0 -l $max_lpc_order -m -e --sample-rate=48000"
	done
	for b in 01 ; do
		test_file sine${bps}-$b 1 $bps "-0 -l $max_lpc_order -m -e --sample-rate=96000"
	done
	for b in 02 03 04 ; do
		test_file sine${bps}-$b 1 $bps "-0 -l $max_lpc_order -m -e"
	done
	for b in 10 11 ; do
		test_file sine${bps}-$b 2 $bps "-0 -l $max_lpc_order -m -e --sample-rate=48000"
	done
	for b in 12 ; do
		test_file sine${bps}-$b 2 $bps "-0 -l $max_lpc_order -m -e --sample-rate=96000"
	done
	for b in 13 14 15 16 17 18 19 ; do
		test_file sine${bps}-$b 2 $bps "-0 -l $max_lpc_order -m -e"
	done
done

echo "Testing blocksize variations..."
for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
	for blocksize in 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 ; do
		for lpc_order in 0 1 2 3 4 5 7 8 9 15 16 17 31 32 ; do
			if [ $lpc_order = 0 ] || [ $lpc_order -le $blocksize ] ; then
					test_file noise8m32 1 8 "-8 -p -e -l $lpc_order --lax --blocksize=$blocksize $disable"
			fi
		done
	done
done

echo "Testing some frame header variations..."
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax -b $max_lpc_order"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax -b 65535"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=9"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=90"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=90000"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=9"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=90"
test_file sine16-01 1 16 "-0 -l $max_lpc_order -m -e -p --lax --sample-rate=90000"

echo "Testing option variations..."
for f in 00 01 02 03 04 ; do
	for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
		for opt in 0 1 2 4 5 6 8 ; do
			for extras in '' '-p' '-e' ; do
				test_file sine16-$f 1 16 "-$opt $extras $disable"
			done
		done
		if [ "$FLAC__EXHAUSTIVE_TESTS" = yes ] ; then
			test_file sine16-$f 1 16 "-b 16384 -m -r 8 -l $max_lpc_order -e -p $disable"
		fi
	done
done

for f in 10 11 12 13 14 15 16 17 18 19 ; do
	for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
		for opt in 0 1 2 4 5 6 8 ; do
			for extras in '' '-p' '-e' ; do
				test_file sine16-$f 2 16 "-$opt $extras $disable"
			done
		done
		if [ "$FLAC__EXHAUSTIVE_TESTS" = yes ] ; then
			test_file sine16-$f 2 16 "-b 16384 -m -r 8 -l $max_lpc_order -e -p $disable"
		fi
	done
done

echo "Testing noise..."
for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
	for channels in 1 2 4 8 ; do
		for bps in 8 16 24 ; do
			for opt in 0 1 2 3 4 5 6 7 8 ; do
				for extras in '' '-p' '-e' ; do
					for blocksize in '' '--lax -b 32' '--lax -b 32768' '--lax -b 65535' ; do
						test_file noise $channels $bps "-$opt $extras $blocksize $disable"
					done
				done
			done
			if [ "$FLAC__EXHAUSTIVE_TESTS" = yes ] ; then
				test_file noise $channels $bps "-b 16384 -m -r 8 -l $max_lpc_order -e -p $disable"
			fi
		done
	done
done
