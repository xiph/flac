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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../src/test_streams:../obj/release/bin:../obj/debug/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || (echo "ERROR can't find flac executable" 1>&2 && exit 1)
if [ $? != 0 ] ; then exit 1 ; fi

run_flac ()
{
	if [ "$FLAC__VALGRIND" = yes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=10 --logfile-fd=4 flac $* 4>>valgrind.log
	else
		flac $*
	fi
}

echo "Checking for --ogg support in flac..."
if flac --ogg --silent --force-raw-format --endian=little --sign=signed --channels=1 --bps=8 --sample-rate=44100 -c $0 1>/dev/null 2>&1 ; then
	has_ogg=yes;
	echo "flac --ogg works"
else
	has_ogg=no;
	echo "flac --ogg doesn't work"
fi

#
# test --skip
#
echo "123456789012345678901234567890123456789" > 39c.raw
echo "012345678901234567890123456789012345678" > 39cu.raw
echo "0123456789012345678901234567890123456789" > 40c.raw
echo "01234567890123456789012345678901234567890123456789" > 50c.raw

# test encode --skip=#
echo "testing --skip=# (encode)"
if run_flac --silent --verify --lax --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1 --skip=10 -o 50c.skip10.flac 50c.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed -o 50c.skip10.raw 50c.skip10.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 40c.raw 50c.skip10.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=10 (encode)" 1>&2
	exit 1
fi
rm -f 50c.skip10.flac 50c.skip10.raw
echo OK

# test encode --skip=mm:ss
echo "testing --skip=mm:ss (encode)"
if run_flac --silent --verify --lax --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1 --skip=0:01 -o 50c.skip0:01.flac 50c.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed -o 50c.skip0:01.raw 50c.skip0:01.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 40c.raw 50c.skip0:01.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=0:01 (encode)" 1>&2
	exit 1
fi
rm -f 50c.skip0:01.flac 50c.skip0:01.raw
echo OK

# test encode --skip=mm:ss.sss
echo "testing --skip=mm:ss.sss (encode)"
if run_flac --silent --verify --lax --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1 --skip=0:01.1 -o 50c.skip0:01.1.flac 50c.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed -o 50c.skip0:01.1.raw 50c.skip0:01.1.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 39c.raw 50c.skip0:01.1.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=0:01.1 (encode)" 1>&2
	exit 1
fi
rm -f 50c.skip0:01.1.flac 50c.skip0:01.1.raw
echo OK

# test decode --skip=#
echo "testing --skip=# (decode)"
if run_flac --silent --verify --lax --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1 -o 50c.flac 50c.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed --skip=10 -o 50c.skip10.raw 50c.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 40c.raw 50c.skip10.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=10 (decode)" 1>&2
	exit 1
fi
rm -f 50c.skip10.raw
echo OK

# test decode --skip=mm:ss
echo "testing --skip=mm:ss (decode)"
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed --skip=0:01 -o 50c.skip0:01.raw 50c.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 40c.raw 50c.skip0:01.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=0:01 (decode)" 1>&2
	exit 1
fi
rm -f 50c.skip0:01.raw
echo OK

# test decode --skip=mm:ss.sss
echo "testing --skip=mm:ss.sss (decode)"
if run_flac --silent --decode --force-raw-format --endian=big --sign=signed --skip=0:01.1 -o 50c.skip0:01.1.raw 50c.flac ; then : ; else
	echo "ERROR decoding FLAC file" 1>&2
	exit 1
fi
if cmp 39c.raw 50c.skip0:01.1.raw ; then : ; else
	echo "ERROR: file mismatch for --skip=0:01.1 (decode)" 1>&2
	exit 1
fi
rm -f 50c.skip0:01.1.raw
echo OK

rm -f 50c.flac


#
# multi-file tests
#

echo "Generating streams..."
if [ ! -f wacky1.wav ] ; then
	if test_streams ; then : ; else
		echo "ERROR during test_streams" 1>&2
		exit 1
	fi
fi

echo "Generating multiple input files from noise..."
if run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 noise.raw ; then : ; else
	echo "ERROR generating FLAC file" 1>&2
	exit 1
fi
if run_flac --decode --silent noise.flac ; then : ; else
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
	sector_align=$2
	encode_options="$3"

	if [ $streamtype = ogg ] ; then
		suffix=ogg
		encode_options="$encode_options --ogg"
	else
		suffix=flac
	fi

	if [ $sector_align = sector_align ] ; then
		encode_options="$encode_options --sector-align"
	fi

	if run_flac $encode_options file0.wav file1.wav file2.wav ; then : ; else
		echo "ERROR" 1>&2
		exit 1
	fi
	for n in 0 1 2 ; do
		mv file$n.$suffix file${n}x.$suffix
	done
	if run_flac --decode file0x.$suffix file1x.$suffix file2x.$suffix ; then : ; else
		echo "ERROR" 1>&2
		exit 1
	fi
	if [ $sector_align != sector_align ] ; then
		for n in 0 1 2 ; do
			if cmp file$n.wav file${n}x.wav ; then : ; else
				echo "ERROR: file mismatch on file #$n" 1>&2
				exit 1
			fi
		done
	fi
	for n in 0 1 2 ; do
		rm -f file${n}x.$suffix file${n}x.wav
	done
}

echo "Testing multiple files without verify..."
test_multifile flac no_sector_align ""

echo "Testing multiple files with verify..."
test_multifile flac no_sector_align "--verify"

echo "Testing multiple files with --sector-align, without verify..."
test_multifile flac sector_align ""

echo "Testing multiple files with --sector-align, with verify..."
test_multifile flac sector_align "--verify"

if [ $has_ogg = "yes" ] ; then
	echo "Testing multiple files with --ogg, without verify..."
	test_multifile ogg no_sector_align ""

	echo "Testing multiple files with --ogg, with verify..."
	test_multifile ogg no_sector_align "--verify"

	echo "Testing multiple files with --ogg and --sector-align, without verify..."
	test_multifile ogg sector_align ""

	echo "Testing multiple files with --ogg and --sector-align, with verify..."
	test_multifile sector_align ogg "--verify"

	echo "Testing multiple files with --ogg and --serial-number, with verify..."
	test_multifile ogg no_sector_align "--serial-number=321 --verify"
fi
