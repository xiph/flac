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
PATH=../src/flac:../src/test_streams:../obj/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || (echo "ERROR can't find flac executable" 1>&2 && exit 1)
if [ $? != 0 ] ; then exit 1 ; fi

echo "Generating streams..."
if test_streams ; then : ; else
	echo "ERROR during test_streams" 1>&2
	exit 1
fi

echo "Checking for --ogg support in flac..."
if flac --ogg wacky1.wav 1>/dev/null 2>&1 ; then
	has_ogg=yes;
	echo "flac --ogg works"
else
	has_ogg=no;
	echo "flac --ogg doesn't work"
fi

#
# multi-file tests
#
echo "Generating multiple input files from noise..."
if flac --verify --silent --force-raw-input --endian=big --sample-rate=44100 --bps=16 --channels=2 noise.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if flac --decode --silent noise.flac ; then : ; else
	echo "ERROR generating WAVE file" 1>&2
	exit 1
fi
rm -f noise.flac
mv noise.wav file0.wav
cp file0.wav file1.wav
cp file1.wav file2.wav

test_multifile ()
{
	streamtype=$1
	encode_options="$2"

	if [ $streamtype = ogg ] ; then
		suffix=ogg
		encode_options="$encode_options --ogg"
	else
		suffix=flac
	fi

	if flac $encode_options file0.wav file1.wav file2.wav ; then : ; else
		echo "ERROR" 1>&2
		exit 1
	fi
	for n in 0 1 2 ; do
		mv file$n.$suffix file${n}x.$suffix
	done
	if flac --decode file0x.$suffix file1x.$suffix file2x.$suffix ; then : ; else
		echo "ERROR" 1>&2
		exit 1
	fi
	for n in 0 1 2 ; do
		if cmp file$n.wav file${n}x.wav ; then : ; else
			echo "ERROR: file mismatch on file #$n" 1>&2
			exit 1
		fi
	done
	for n in 0 1 2 ; do
		rm -f file${n}x.$suffix file${n}x.wav
	done
}

echo "Testing multiple files without verify..."
test_multifile flac ""

echo "Testing multiple files with verify..."
test_multifile flac "--verify"

#@@@@echo "Testing multiple files with --sector-align, without verify..."
#@@@@test_multifile flac "--sector-align"

#@@@@echo "Testing multiple files with --sector-align, with verify..."
#@@@@test_multifile flac "--sector-align --verify"

if [ $has_ogg = "yes" ] ; then
	echo "Testing multiple files with --ogg, without verify..."
	test_multifile ogg ""

	echo "Testing multiple files with --ogg, with verify..."
	test_multifile ogg "--verify"

	#@@@@echo "Testing multiple files with --ogg and --sector-align, without verify..."
	#@@@@test_multifile ogg "--sector-align"

	#@@@@echo "Testing multiple files with --ogg and --sector-align, with verify..."
	#@@@@test_multifile ogg "--sector-align --verify"
fi

#
# single-file tests
#

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name: encode..."
	cmd="flac --verify --silent --force-raw-input --endian=big --sample-rate=44100 --bps=$bps --channels=$channels $encode_options $name.raw"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	if $cmd 2>>./streams.log ; then : ; else
		echo "ERROR during encode of $name" 1>&2
		exit 1
	fi
	echo -n "decode..."
	cmd="flac --silent --endian=big --decode --force-raw-input --output-name=$name.cmp $name.flac"
	echo "### DECODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	if $cmd 2>>./streams.log ; then : ; else
		echo "ERROR during decode of $name" 1>&2
		exit 1
	fi
	ls -1l $name.raw >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.cmp >> ./streams.log
	echo -n "compare..."
	if cmp $name.raw $name.cmp ; then : ; else
		echo "ERROR during compare of $name" 1>&2
		exit 1
	fi
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
		cmd="flac --verify --silent --force-raw-input --endian=big --sample-rate=44100 --bps=$bps --channels=$channels $encode_options --stdout $name.raw"
		echo "### ENCODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		if $cmd 1>$name.flac 2>>./streams.log ; then : ; else
			echo "ERROR during encode of $name" 1>&2
			exit 1
		fi
	else
		cmd="flac --verify --silent --force-raw-input --endian=big --sample-rate=44100 --bps=$bps --channels=$channels $encode_options --stdout -"
		echo "### ENCODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		if cat $name.raw | $cmd 1>$name.flac 2>>./streams.log ; then : ; else
			echo "ERROR during encode of $name" 1>&2
			exit 1
		fi
	fi
	echo -n "decode via pipes..."
	if [ $is_win = yes ] ; then
		cmd="flac --silent --endian=big --decode --force-raw-input --stdout $name.flac"
		echo "### DECODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		if $cmd 1>$name.cmp 2>>./streams.log ; then : ; else
			echo "ERROR during decode of $name" 1>&2
			exit 1
		fi
	else
		cmd="flac --silent --endian=big --decode --force-raw-input --stdout -"
		echo "### DECODE $name #######################################################" >> ./streams.log
		echo "###    cmd=$cmd" >> ./streams.log
		if cat $name.flac | $cmd 1>$name.cmp 2>>./streams.log ; then : ; else
			echo "ERROR during decode of $name" 1>&2
			exit 1
		fi
	fi
	ls -1l $name.raw >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.cmp >> ./streams.log
	echo -n "compare..."
	if cmp $name.raw $name.cmp ; then : ; else
		echo "ERROR during compare of $name" 1>&2
		exit 1
	fi
	echo OK
}

echo "Testing noise through pipes..."
test_file_piped noise 1 8 "-0"

echo "Testing small files..."
test_file test01 1 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
test_file test02 2 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
test_file test03 1 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
test_file test04 2 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"

echo "Testing 8-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd8-$b 1 8 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
done

echo "Testing 16-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd16-$b 1 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
done

echo "Testing 24-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd24-$b 1 24 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
done

echo "Testing 16-bit wasted-bits-per-sample streams..."
for b in 01 ; do
	test_file wbps16-$b 1 16 "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search --qlp-coeff-precision-search"
done

for bps in 16 24 ; do
	echo "Testing $bps-bit sine wave streams..."
	for b in 00 01 02 03 04 ; do
		test_file sine${bps}-$b 1 $bps "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search"
	done
	for b in 10 11 12 13 14 15 16 17 18 19 ; do
		test_file sine${bps}-$b 2 $bps "-0 --max-lpc-order=16 --mid-side --exhaustive-model-search"
	done
done

echo "Testing some frame header variations..."
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --lax --blocksize=16"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --lax --blocksize=65535"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --blocksize=16"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --blocksize=65535"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --lax --sample-rate=9"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --lax --sample-rate=90"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --lax --sample-rate=90000"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --sample-rate=9"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --sample-rate=90"
test_file sine16-01 1 16 "-0 --max-lpc-order=8 --mid-side --exhaustive-model-search --qlp-coeff-precision-search --sample-rate=90000"

echo "Testing option variations..."
for f in 00 01 02 03 04 ; do
	for opt in 0 1 2 4 5 6 8 ; do
		for extras in '' '--qlp-coeff-precision-search' '--exhaustive-model-search' ; do
			test_file sine16-$f 1 16 "-$opt $extras"
		done
	done
done
for f in 10 11 12 13 14 15 16 17 18 19 ; do
	for opt in 0 1 2 4 5 6 8 ; do
		for extras in '' '--qlp-coeff-precision-search' '--exhaustive-model-search' ; do
			test_file sine16-$f 2 16 "-$opt $extras"
		done
	done
done

echo "Testing noise..."
for opt in 0 1 2 3 4 5 6 7 8 ; do
	for extras in '' '--qlp-coeff-precision-search' '--exhaustive-model-search' ; do
		for blocksize in '' '--blocksize=32' '--blocksize=32768' '--blocksize=65535' ; do
			for channels in 1 2 4 8 ; do
				for bps in 8 16 24 ; do
					test_file noise $channels $bps "-$opt $extras $blocksize"
				done
			done
		done
	done
done
