#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004  Josh Coalson
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

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/release/lib:../obj/debug/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../src/test_streams:../obj/release/bin:../obj/debug/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 flac $* 4>>test_flac.valgrind.log
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


echo "Generating streams..."
if [ ! -f wacky1.wav ] ; then
	test_streams || die "ERROR during test_streams"
fi

############################################################################
# basic 'round-trip' tests of various kinds of streams
############################################################################

rt_test_raw ()
{
	f="$1"
	channels=`echo $f | awk -F- '{print $2}'`
	bytes_per_sample=`echo $f | awk -F- '{print $3}'`
	bps=`expr $bytes_per_sample '*' 8`
	echo -n "round-trip test ($f) encode... "
	run_flac --silent --verify --force-raw-format --endian=little --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --decode --force-raw-format --endian=little --sign=signed -o rt.raw rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.raw || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.raw
}

rt_test_wav ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac --silent --verify $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --decode -o rt.wav rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.wav || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.wav
}

rt_test_aiff ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac --silent --verify $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --decode -o rt.aiff rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.aiff || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.aiff
}

for f in rt-*.raw ; do
	rt_test_raw $f
done
for f in rt-*.wav ; do
	rt_test_wav $f
done
for f in rt-*.aiff ; do
	rt_test_aiff $f
done

############################################################################
# test --skip and --until
############################################################################

#
# first make some chopped-up raw files
#
echo "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMN" > master.raw
dddie="die ERROR: creating files for --skip/--until tests"
dd if=master.raw ibs=1 count=50 of=50c.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=40 of=50c.skip10.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=11 count=39 of=50c.skip11.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=40 of=50c.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=39 of=50c.until39.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=30 of=50c.skip10.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=29 of=50c.skip10.until39.raw 2>/dev/null || $dddie

wav_eopt="--silent --verify --lax"
wav_dopt="--silent --decode"

raw_eopt="$wav_eopt --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1"
raw_dopt="$wav_dopt --force-raw-format --endian=big --sign=signed"

#
# convert them to WAVE and AIFF files
#
convert_to_wav ()
{
	run_flac $raw_eopt $1.raw || die "ERROR converting $1.raw to WAVE"
	run_flac $wav_dopt $1.flac || die "ERROR converting $1.raw to WAVE"
}
convert_to_wav 50c
convert_to_wav 50c.skip10
convert_to_wav 50c.skip11
convert_to_wav 50c.until40
convert_to_wav 50c.until39
convert_to_wav 50c.skip10.until40
convert_to_wav 50c.skip10.until39

convert_to_aiff ()
{
	run_flac $raw_eopt $1.raw || die "ERROR converting $1.raw to AIFF"
	run_flac $wav_dopt $1.flac -o $1.aiff || die "ERROR converting $1.raw to AIFF"
}
convert_to_aiff 50c
convert_to_aiff 50c.skip10
convert_to_aiff 50c.skip11
convert_to_aiff 50c.until40
convert_to_aiff 50c.until39
convert_to_aiff 50c.skip10.until40
convert_to_aiff 50c.skip10.until39

test_skip_until ()
{
	fmt=$1

	[ $fmt = wav ] || [ $fmt = aiff ] || [ $fmt = raw ] || die "ERROR: internal error, bad format '$fmt'"

	if [ $fmt = raw ] ; then
		eopt="$raw_eopt"
		dopt="$raw_dopt"
	else
		eopt="$wav_eopt"
		dopt="$wav_dopt"
	fi

	#
	# test --skip when encoding
	#

	echo -n "testing --skip=# (encode) ($fmt)... "
	run_flac $eopt --skip=10 -o z50c.skip10.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.$fmt z50c.skip10.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.$fmt z50c.skip10.$fmt || die "ERROR: file mismatch for --skip=10 (encode) ($fmt)"
	rm -f z50c.skip10.flac z50c.skip10.$fmt
	echo OK

	echo -n "testing --skip=mm:ss (encode) ($fmt)... "
	run_flac $eopt --skip=0:01 -o z50c.skip0:01.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip0:01.$fmt z50c.skip0:01.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.$fmt z50c.skip0:01.$fmt || die "ERROR: file mismatch for --skip=0:01 (encode) ($fmt)"
	rm -f z50c.skip0:01.flac z50c.skip0:01.$fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --skip=0:01.1001 -o z50c.skip0:01.1001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip0:01.1001.$fmt z50c.skip0:01.1001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip11.$fmt z50c.skip0:01.1001.$fmt || die "ERROR: file mismatch for --skip=0:01.1001 (encode) ($fmt)"
	rm -f z50c.skip0:01.1001.flac z50c.skip0:01.1001.$fmt
	echo OK

	#
	# test --skip when decoding
	#

	echo -n "testing --skip=# (decode) ($fmt)... "
	run_flac $eopt -o z50c.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt --skip=10 -o z50c.skip10.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.$fmt z50c.skip10.$fmt || die "ERROR: file mismatch for --skip=10 (decode) ($fmt)"
	rm -f z50c.skip10.$fmt
	echo OK

	echo -n "testing --skip=mm:ss (decode) ($fmt)... "
	run_flac $dopt --skip=0:01 -o z50c.skip0:01.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.$fmt z50c.skip0:01.$fmt || die "ERROR: file mismatch for --skip=0:01 (decode) ($fmt)"
	rm -f z50c.skip0:01.$fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (decode) ($fmt)... "
	run_flac $dopt --skip=0:01.1001 -o z50c.skip0:01.1001.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip11.$fmt z50c.skip0:01.1001.$fmt || die "ERROR: file mismatch for --skip=0:01.1001 (decode) ($fmt)"
	rm -f z50c.skip0:01.1001.$fmt
	echo OK

	rm -f z50c.flac

	#
	# test --until when encoding
	#

	echo -n "testing --until=# (encode) ($fmt)... "
	run_flac $eopt --until=40 -o z50c.until40.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until40.$fmt z50c.until40.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until40.$fmt || die "ERROR: file mismatch for --until=40 (encode) ($fmt)"
	rm -f z50c.until40.flac z50c.until40.$fmt
	echo OK

	echo -n "testing --until=mm:ss (encode) ($fmt)... "
	run_flac $eopt --until=0:04 -o z50c.until0:04.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until0:04.$fmt z50c.until0:04.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until0:04.$fmt || die "ERROR: file mismatch for --until=0:04 (encode) ($fmt)"
	rm -f z50c.until0:04.flac z50c.until0:04.$fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --until=0:03.9001 -o z50c.until0:03.9001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until0:03.9001.$fmt z50c.until0:03.9001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until39.$fmt z50c.until0:03.9001.$fmt || die "ERROR: file mismatch for --until=0:03.9001 (encode) ($fmt)"
	rm -f z50c.until0:03.9001.flac z50c.until0:03.9001.$fmt
	echo OK

	echo -n "testing --until=-# (encode) ($fmt)... "
	run_flac $eopt --until=-10 -o z50c.until-10.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until-10.$fmt z50c.until-10.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until-10.$fmt || die "ERROR: file mismatch for --until=-10 (encode) ($fmt)"
	rm -f z50c.until-10.flac z50c.until-10.$fmt
	echo OK

	echo -n "testing --until=-mm:ss (encode) ($fmt)... "
	run_flac $eopt --until=-0:01 -o z50c.until-0:01.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until-0:01.$fmt z50c.until-0:01.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until-0:01.$fmt || die "ERROR: file mismatch for --until=-0:01 (encode) ($fmt)"
	rm -f z50c.until-0:01.flac z50c.until-0:01.$fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --until=-0:01.1001 -o z50c.until-0:01.1001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.until-0:01.1001.$fmt z50c.until-0:01.1001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until39.$fmt z50c.until-0:01.1001.$fmt || die "ERROR: file mismatch for --until=-0:01.1001 (encode) ($fmt)"
	rm -f z50c.until-0:01.1001.flac z50c.until-0:01.1001.$fmt
	echo OK

	#
	# test --until when decoding
	#

	run_flac $eopt -o z50c.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"

	echo -n "testing --until=# (decode) ($fmt)... "
	run_flac $dopt --until=40 -o z50c.until40.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until40.$fmt || die "ERROR: file mismatch for --until=40 (decode) ($fmt)"
	rm -f z50c.until40.$fmt
	echo OK

	echo -n "testing --until=mm:ss (decode) ($fmt)... "
	run_flac $dopt --until=0:04 -o z50c.until0:04.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until0:04.$fmt || die "ERROR: file mismatch for --until=0:04 (decode) ($fmt)"
	rm -f z50c.until0:04.$fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (decode) ($fmt)... "
	run_flac $dopt --until=0:03.9001 -o z50c.until0:03.9001.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until39.$fmt z50c.until0:03.9001.$fmt || die "ERROR: file mismatch for --until=0:03.9001 (decode) ($fmt)"
	rm -f z50c.until0:03.9001.$fmt
	echo OK

	echo -n "testing --until=-# (decode) ($fmt)... "
	run_flac $dopt --until=-10 -o z50c.until-10.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until-10.$fmt || die "ERROR: file mismatch for --until=-10 (decode) ($fmt)"
	rm -f z50c.until-10.$fmt
	echo OK

	echo -n "testing --until=-mm:ss (decode) ($fmt)... "
	run_flac $dopt --until=-0:01 -o z50c.until-0:01.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until40.$fmt z50c.until-0:01.$fmt || die "ERROR: file mismatch for --until=-0:01 (decode) ($fmt)"
	rm -f z50c.until-0:01.$fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (decode) ($fmt)... "
	run_flac $dopt --until=-0:01.1001 -o z50c.until-0:01.1001.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.until39.$fmt z50c.until-0:01.1001.$fmt || die "ERROR: file mismatch for --until=-0:01.1001 (decode) ($fmt)"
	rm -f z50c.until-0:01.1001.$fmt
	echo OK

	rm -f z50c.flac

	#
	# test --skip and --until when encoding
	#

	echo -n "testing --skip=10 --until=# (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=40 -o z50c.skip10.until40.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until40.$fmt z50c.skip10.until40.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until40.$fmt || die "ERROR: file mismatch for --skip=10 --until=40 (encode) ($fmt)"
	rm -f z50c.skip10.until40.flac z50c.skip10.until40.$fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until0:04.$fmt z50c.skip10.until0:04.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until0:04.$fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (encode) ($fmt)"
	rm -f z50c.skip10.until0:04.flac z50c.skip10.until0:04.$fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until0:03.9001.$fmt z50c.skip10.until0:03.9001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until39.$fmt z50c.skip10.until0:03.9001.$fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (encode) ($fmt)"
	rm -f z50c.skip10.until0:03.9001.flac z50c.skip10.until0:03.9001.$fmt
	echo OK

	echo -n "testing --skip=10 --until=+# (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=+30 -o z50c.skip10.until+30.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until+30.$fmt z50c.skip10.until+30.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until+30.$fmt || die "ERROR: file mismatch for --skip=10 --until=+30 (encode) ($fmt)"
	rm -f z50c.skip10.until+30.flac z50c.skip10.until+30.$fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=+0:03 -o z50c.skip10.until+0:03.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until+0:03.$fmt z50c.skip10.until+0:03.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until+0:03.$fmt || die "ERROR: file mismatch for --skip=10 --until=+0:03 (encode) ($fmt)"
	rm -f z50c.skip10.until+0:03.flac z50c.skip10.until+0:03.$fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=+0:02.9001 -o z50c.skip10.until+0:02.9001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until+0:02.9001.$fmt z50c.skip10.until+0:02.9001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until39.$fmt z50c.skip10.until+0:02.9001.$fmt || die "ERROR: file mismatch for --skip=10 --until=+0:02.9001 (encode) ($fmt)"
	rm -f z50c.skip10.until+0:02.9001.flac z50c.skip10.until+0:02.9001.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=-10 -o z50c.skip10.until-10.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until-10.$fmt z50c.skip10.until-10.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until-10.$fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (encode) ($fmt)"
	rm -f z50c.skip10.until-10.flac z50c.skip10.until-10.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until-0:01.$fmt z50c.skip10.until-0:01.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until-0:01.$fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (encode) ($fmt)"
	rm -f z50c.skip10.until-0:01.flac z50c.skip10.until-0:01.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (encode) ($fmt)... "
	run_flac $eopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"
	run_flac $dopt -o z50c.skip10.until-0:01.1001.$fmt z50c.skip10.until-0:01.1001.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until39.$fmt z50c.skip10.until-0:01.1001.$fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (encode) ($fmt)"
	rm -f z50c.skip10.until-0:01.1001.flac z50c.skip10.until-0:01.1001.$fmt
	echo OK

	#
	# test --skip and --until when decoding
	#

	run_flac $eopt -o z50c.flac 50c.$fmt || die "ERROR generating FLAC file ($fmt)"

	echo -n "testing --skip=10 --until=# (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=40 -o z50c.skip10.until40.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until40.$fmt || die "ERROR: file mismatch for --skip=10 --until=40 (decode) ($fmt)"
	rm -f z50c.skip10.until40.$fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until0:04.$fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (decode) ($fmt)"
	rm -f z50c.skip10.until0:04.$fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until39.$fmt z50c.skip10.until0:03.9001.$fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (decode) ($fmt)"
	rm -f z50c.skip10.until0:03.9001.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=-10 -o z50c.skip10.until-10.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until-10.$fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (decode) ($fmt)"
	rm -f z50c.skip10.until-10.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until40.$fmt z50c.skip10.until-0:01.$fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (decode) ($fmt)"
	rm -f z50c.skip10.until-0:01.$fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (decode) ($fmt)... "
	run_flac $dopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.$fmt z50c.flac || die "ERROR decoding FLAC file ($fmt)"
	cmp 50c.skip10.until39.$fmt z50c.skip10.until-0:01.1001.$fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (decode) ($fmt)"
	rm -f z50c.skip10.until-0:01.1001.$fmt
	echo OK

	rm -f z50c.flac
}

test_skip_until raw
test_skip_until wav
test_skip_until aiff


############################################################################
# test 'fixup' code that happens when a FLAC file with total_samples == 0
# in the STREAMINFO block is converted to WAVE or AIFF, requiring the
# decoder go back and fix up the chunk headers
############################################################################

echo -n "WAVE fixup test... "
echo -n "prepare... "
convert_to_wav noise || die "ERROR creating reference WAVE"
echo -n "encode... "
cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
echo -n "decode... "
run_flac $wav_dopt fixup.flac -o fixup.wav || die "ERROR decoding FLAC file"
echo -n "compare... "
cmp noise.wav fixup.wav || die "ERROR: file mismatch"
echo OK
rm -f noise.wav fixup.wav fixup.flac

echo -n "AIFF fixup test... "
echo -n "prepare... "
convert_to_aiff noise || die "ERROR creating reference AIFF"
echo -n "encode... "
cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
echo -n "decode... "
run_flac $wav_dopt fixup.flac -o fixup.aiff || die "ERROR decoding FLAC file"
echo -n "compare... "
cmp noise.aiff fixup.aiff || die "ERROR: file mismatch"
echo OK
rm -f noise.aiff fixup.aiff fixup.flac


############################################################################
# multi-file tests
############################################################################

echo "Generating multiple input files from noise..."
run_flac --verify --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 noise.raw || die "ERROR generating FLAC file"
run_flac --decode --silent noise.flac || die "ERROR generating WAVE file"
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

	run_flac $encode_options file0.wav file1.wav file2.wav || die "ERROR"
	for n in 0 1 2 ; do
		mv file$n.$suffix file${n}x.$suffix
	done
	run_flac --decode file0x.$suffix file1x.$suffix file2x.$suffix || die "ERROR"
	if [ $sector_align != sector_align ] ; then
		for n in 0 1 2 ; do
			cmp file$n.wav file${n}x.wav || die "ERROR: file mismatch on file #$n"
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
