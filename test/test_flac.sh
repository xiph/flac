#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004,2005  Josh Coalson
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

if [ x = x"$1" ] ; then 
	BUILD=debug
else
	BUILD="$1"
fi

LD_LIBRARY_PATH=../src/libFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/libOggFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/grabbag/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/getopt/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/replaygain_analysis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/replaygain_synthesis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/utf8/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../obj/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:$PATH
PATH=../src/metaflac:$PATH
PATH=../src/test_streams:$PATH
PATH=../obj/$BUILD/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 flac $* 4>>test_flac.valgrind.log
	else
		flac $*
	fi
}

run_metaflac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 metaflac $* 4>>test_flac.valgrind.log
	else
		metaflac $*
	fi
}

if [ `env | grep -ic '^comspec='` != 0 ] ; then
	is_win=yes
else
	is_win=no
fi

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
# test that flac doesn't automatically overwrite files unless -f is used
############################################################################

echo "Try encoding to a file that exists; should fail"
cp wacky1.wav exist.wav
touch exist.flac
if run_flac --totally-silent -0 exist.wav ; then
	die "ERROR: it should have failed but didn't"
else
	echo "OK, it failed as it should"
fi

echo "Try encoding with -f to a file that exists; should succeed"
if run_flac --totally-silent -0 --force exist.wav ; then
	echo "OK, it succeeded as it should"
else
	die "ERROR: it should have succeeded but didn't"
fi

echo "Try decoding to a file that exists; should fail"
if run_flac --totally-silent -d exist.flac ; then
	die "ERROR: it should have failed but didn't"
else
	echo "OK, it failed as it should"
fi

echo "Try decoding with -f to a file that exists; should succeed"
if run_flac --totally-silent -d -f exist.flac ; then
	echo "OK, it succeeded as it should"
else
	die "ERROR: it should have succeeded but didn't"
fi

rm -f exist.wav exist.flac

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
	run_flac --silent --force --verify --force-raw-format --endian=little --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --force --decode --force-raw-format --endian=little --sign=signed -o rt.raw rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.raw || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.raw
}

rt_test_wav ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac --silent --force --verify $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --force --decode -o rt.wav rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.wav || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.wav
}

rt_test_aiff ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac --silent --force --verify $f -o rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac --silent --force --decode -o rt.aiff rt.flac || die "ERROR"
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
dd if=master.raw ibs=1 skip=20 count=30 of=50c.skip20.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=30 count=20 of=50c.skip30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=40 count=10 of=50c.skip40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=10 of=50c.until10.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=20 of=50c.until20.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=30 of=50c.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=39 of=50c.until39.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=40 of=50c.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=20 of=50c.skip10.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=29 of=50c.skip10.until39.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=30 of=50c.skip10.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=20 count=10 of=50c.skip20.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=20 count=20 of=50c.skip20.until40.raw 2>/dev/null || $dddie

wav_eopt="--silent --force --verify --lax"
wav_dopt="--silent --force --decode"

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
convert_to_wav 50c.skip20
convert_to_wav 50c.skip30
convert_to_wav 50c.skip40
convert_to_wav 50c.until10
convert_to_wav 50c.until20
convert_to_wav 50c.until30
convert_to_wav 50c.until39
convert_to_wav 50c.until40
convert_to_wav 50c.skip10.until30
convert_to_wav 50c.skip10.until39
convert_to_wav 50c.skip10.until40
convert_to_wav 50c.skip20.until30
convert_to_wav 50c.skip20.until40

convert_to_aiff ()
{
	run_flac $raw_eopt $1.raw || die "ERROR converting $1.raw to AIFF"
	run_flac $wav_dopt $1.flac -o $1.aiff || die "ERROR converting $1.raw to AIFF"
}
convert_to_aiff 50c
convert_to_aiff 50c.skip10
convert_to_aiff 50c.skip11
convert_to_aiff 50c.skip20
convert_to_aiff 50c.skip30
convert_to_aiff 50c.skip40
convert_to_aiff 50c.until10
convert_to_aiff 50c.until20
convert_to_aiff 50c.until30
convert_to_aiff 50c.until39
convert_to_aiff 50c.until40
convert_to_aiff 50c.skip10.until30
convert_to_aiff 50c.skip10.until39
convert_to_aiff 50c.skip10.until40
convert_to_aiff 50c.skip20.until30
convert_to_aiff 50c.skip20.until40

test_skip_until ()
{
	in_fmt=$1
	out_fmt=$2

	[ "$in_fmt" = wav ] || [ "$in_fmt" = aiff ] || [ "$in_fmt" = raw ] || die "ERROR: internal error, bad 'in' format '$in_fmt'"

	[ "$out_fmt" = flac ] || [ "$out_fmt" = ogg ] || die "ERROR: internal error, bad 'out' format '$out_fmt'"

	if [ $in_fmt = raw ] ; then
		eopt="$raw_eopt"
		dopt="$raw_dopt"
	else
		eopt="$wav_eopt"
		dopt="$wav_dopt"
	fi

	if [ $out_fmt = ogg ] ; then
		eopt="--ogg $eopt"
	fi

	#
	# test --skip when encoding
	#

	desc="($in_fmt<->$out_fmt)"

	echo -n "testing --skip=# (encode) $desc... "
	run_flac $eopt --skip=10 -o z50c.skip10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.$in_fmt z50c.skip10.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.skip10.$in_fmt || die "ERROR: file mismatch for --skip=10 (encode) $desc"
	rm -f z50c.skip10.$out_fmt z50c.skip10.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss (encode) $desc... "
	run_flac $eopt --skip=0:01 -o z50c.skip0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip0:01.$in_fmt z50c.skip0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.skip0:01.$in_fmt || die "ERROR: file mismatch for --skip=0:01 (encode) $desc"
	rm -f z50c.skip0:01.$out_fmt z50c.skip0:01.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=0:01.1001 -o z50c.skip0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip0:01.1001.$in_fmt z50c.skip0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip11.$in_fmt z50c.skip0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=0:01.1001 (encode) $desc"
	rm -f z50c.skip0:01.1001.$out_fmt z50c.skip0:01.1001.$in_fmt
	echo OK

	#
	# test --skip when decoding
	#

	echo -n "testing --skip=# (decode) $desc... "
	run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt --skip=10 -o z50c.skip10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.skip10.$in_fmt || die "ERROR: file mismatch for --skip=10 (decode) $desc"
	rm -f z50c.skip10.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss (decode) $desc... "
	run_flac $dopt --skip=0:01 -o z50c.skip0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.skip0:01.$in_fmt || die "ERROR: file mismatch for --skip=0:01 (decode) $desc"
	rm -f z50c.skip0:01.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=0:01.1001 -o z50c.skip0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip11.$in_fmt z50c.skip0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=0:01.1001 (decode) $desc"
	rm -f z50c.skip0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt

	#
	# test --until when encoding
	#

	echo -n "testing --until=# (encode) $desc... "
	run_flac $eopt --until=40 -o z50c.until40.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until40.$in_fmt z50c.until40.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until40.$in_fmt || die "ERROR: file mismatch for --until=40 (encode) $desc"
	rm -f z50c.until40.$out_fmt z50c.until40.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss (encode) $desc... "
	run_flac $eopt --until=0:04 -o z50c.until0:04.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until0:04.$in_fmt z50c.until0:04.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until0:04.$in_fmt || die "ERROR: file mismatch for --until=0:04 (encode) $desc"
	rm -f z50c.until0:04.$out_fmt z50c.until0:04.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (encode) $desc... "
	run_flac $eopt --until=0:03.9001 -o z50c.until0:03.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until0:03.9001.$in_fmt z50c.until0:03.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until39.$in_fmt z50c.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --until=0:03.9001 (encode) $desc"
	rm -f z50c.until0:03.9001.$out_fmt z50c.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --until=-# (encode) $desc... "
	run_flac $eopt --until=-10 -o z50c.until-10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until-10.$in_fmt z50c.until-10.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until-10.$in_fmt || die "ERROR: file mismatch for --until=-10 (encode) $desc"
	rm -f z50c.until-10.$out_fmt z50c.until-10.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss (encode) $desc... "
	run_flac $eopt --until=-0:01 -o z50c.until-0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until-0:01.$in_fmt z50c.until-0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until-0:01.$in_fmt || die "ERROR: file mismatch for --until=-0:01 (encode) $desc"
	rm -f z50c.until-0:01.$out_fmt z50c.until-0:01.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (encode) $desc... "
	run_flac $eopt --until=-0:01.1001 -o z50c.until-0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.until-0:01.1001.$in_fmt z50c.until-0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until39.$in_fmt z50c.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --until=-0:01.1001 (encode) $desc"
	rm -f z50c.until-0:01.1001.$out_fmt z50c.until-0:01.1001.$in_fmt
	echo OK

	#
	# test --until when decoding
	#

	run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"

	echo -n "testing --until=# (decode) $desc... "
	run_flac $dopt --until=40 -o z50c.until40.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until40.$in_fmt || die "ERROR: file mismatch for --until=40 (decode) $desc"
	rm -f z50c.until40.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss (decode) $desc... "
	run_flac $dopt --until=0:04 -o z50c.until0:04.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until0:04.$in_fmt || die "ERROR: file mismatch for --until=0:04 (decode) $desc"
	rm -f z50c.until0:04.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (decode) $desc... "
	run_flac $dopt --until=0:03.9001 -o z50c.until0:03.9001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until39.$in_fmt z50c.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --until=0:03.9001 (decode) $desc"
	rm -f z50c.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --until=-# (decode) $desc... "
	run_flac $dopt --until=-10 -o z50c.until-10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until-10.$in_fmt || die "ERROR: file mismatch for --until=-10 (decode) $desc"
	rm -f z50c.until-10.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss (decode) $desc... "
	run_flac $dopt --until=-0:01 -o z50c.until-0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.until-0:01.$in_fmt || die "ERROR: file mismatch for --until=-0:01 (decode) $desc"
	rm -f z50c.until-0:01.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (decode) $desc... "
	run_flac $dopt --until=-0:01.1001 -o z50c.until-0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until39.$in_fmt z50c.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --until=-0:01.1001 (decode) $desc"
	rm -f z50c.until-0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt

	#
	# test --skip and --until when encoding
	#

	echo -n "testing --skip=10 --until=# (encode) $desc... "
	run_flac $eopt --skip=10 --until=40 -o z50c.skip10.until40.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until40.$in_fmt z50c.skip10.until40.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until40.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=40 (encode) $desc"
	rm -f z50c.skip10.until40.$out_fmt z50c.skip10.until40.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until0:04.$in_fmt z50c.skip10.until0:04.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until0:04.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (encode) $desc"
	rm -f z50c.skip10.until0:04.$out_fmt z50c.skip10.until0:04.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until0:03.9001.$in_fmt z50c.skip10.until0:03.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until39.$in_fmt z50c.skip10.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (encode) $desc"
	rm -f z50c.skip10.until0:03.9001.$out_fmt z50c.skip10.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+# (encode) $desc... "
	run_flac $eopt --skip=10 --until=+30 -o z50c.skip10.until+30.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until+30.$in_fmt z50c.skip10.until+30.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until+30.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+30 (encode) $desc"
	rm -f z50c.skip10.until+30.$out_fmt z50c.skip10.until+30.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=+0:03 -o z50c.skip10.until+0:03.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until+0:03.$in_fmt z50c.skip10.until+0:03.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until+0:03.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+0:03 (encode) $desc"
	rm -f z50c.skip10.until+0:03.$out_fmt z50c.skip10.until+0:03.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=+0:02.9001 -o z50c.skip10.until+0:02.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until+0:02.9001.$in_fmt z50c.skip10.until+0:02.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until39.$in_fmt z50c.skip10.until+0:02.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+0:02.9001 (encode) $desc"
	rm -f z50c.skip10.until+0:02.9001.$out_fmt z50c.skip10.until+0:02.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (encode) $desc... "
	run_flac $eopt --skip=10 --until=-10 -o z50c.skip10.until-10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until-10.$in_fmt z50c.skip10.until-10.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until-10.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (encode) $desc"
	rm -f z50c.skip10.until-10.$out_fmt z50c.skip10.until-10.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until-0:01.$in_fmt z50c.skip10.until-0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until-0:01.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (encode) $desc"
	rm -f z50c.skip10.until-0:01.$out_fmt z50c.skip10.until-0:01.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	run_flac $dopt -o z50c.skip10.until-0:01.1001.$in_fmt z50c.skip10.until-0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until39.$in_fmt z50c.skip10.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (encode) $desc"
	rm -f z50c.skip10.until-0:01.1001.$out_fmt z50c.skip10.until-0:01.1001.$in_fmt
	echo OK

	#
	# test --skip and --until when decoding
	#

	run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"

	echo -n "testing --skip=10 --until=# (decode) $desc... "
	run_flac $dopt --skip=10 --until=40 -o z50c.skip10.until40.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until40.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=40 (decode) $desc"
	rm -f z50c.skip10.until40.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (decode) $desc... "
	run_flac $dopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until0:04.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (decode) $desc"
	rm -f z50c.skip10.until0:04.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until39.$in_fmt z50c.skip10.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (decode) $desc"
	rm -f z50c.skip10.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (decode) $desc... "
	run_flac $dopt --skip=10 --until=-10 -o z50c.skip10.until-10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until-10.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (decode) $desc"
	rm -f z50c.skip10.until-10.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (decode) $desc... "
	run_flac $dopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.skip10.until-0:01.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (decode) $desc"
	rm -f z50c.skip10.until-0:01.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until39.$in_fmt z50c.skip10.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (decode) $desc"
	rm -f z50c.skip10.until-0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt
}

test_skip_until raw flac
test_skip_until wav flac
test_skip_until aiff flac

if [ $has_ogg = "yes" ] ; then
	test_skip_until raw ogg
	test_skip_until wav ogg
	test_skip_until aiff ogg
fi

echo "testing seek extremes:"

run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 --blocksize=576 noise.raw || die "ERROR generating FLAC file"

if [ $is_win = no ] ; then
	total_samples=`run_metaflac --show-total-samples noise.flac`
	[ $? = 0 ] || die "ERROR getting total sample count from noise.flac"
else
	# some flavors of cygwin don't seem to treat the \x0d as a word
	# separator, so we hard code it.  we'll just have to fix it later
	# if we change the way noise.flac is made.
	total_samples=393216
fi

echo -n "testing --skip=0... "
run_flac $wav_dopt --skip=0 -o z.wav noise.flac || die "ERROR decoding FLAC file noise.flac"
echo OK

for delta in 2 1 ; do
	n=`expr $total_samples - $delta`
	echo -n "testing --skip=$n... "
	run_flac $wav_dopt --skip=$n -o z.wav noise.flac || die "ERROR decoding FLAC file noise.flac"
	echo OK
done

rm noise.flac z.wav


############################################################################
# test --input-size
############################################################################

echo -n "testing --input-size=50 --skip=10... "
cat 50c.raw | run_flac $raw_eopt --input-size=50 --skip=10 -o z50c.skip10.flac - || die "ERROR generating FLAC file"
run_flac $raw_dopt -o z50c.skip10.raw z50c.skip10.flac || die "ERROR decoding FLAC file"
cmp 50c.skip10.raw z50c.skip10.raw || die "ERROR: file mismatch for --input-size=50 --skip=10"
rm -f z50c.skip10.raw z50c.skip10.flac
echo OK


############################################################################
# test --skip and --until
############################################################################

############################################################################
# test --cue
############################################################################

#
# create the cue sheet
#
cuesheet=cuetest.cue
cat > $cuesheet << EOF
CATALOG 1234567890123
FILE "blah" WAVE
  TRACK 01 AUDIO
    INDEX 01 0
    INDEX 02 10
    INDEX 03 20
  TRACK 02 AUDIO
    INDEX 01 30
  TRACK 04 AUDIO
    INDEX 01 40
EOF

test_cue ()
{
	in_fmt=$1
	out_fmt=$2

	[ "$in_fmt" = wav ] || [ "$in_fmt" = aiff ] || [ "$in_fmt" = raw ] || die "ERROR: internal error, bad 'in' format '$in_fmt'"

	[ "$out_fmt" = flac ] || [ "$out_fmt" = ogg ] || die "ERROR: internal error, bad 'out' format '$out_fmt'"

	if [ $in_fmt = raw ] ; then
		eopt="$raw_eopt"
		dopt="$raw_dopt"
	else
		eopt="$wav_eopt"
		dopt="$wav_dopt"
	fi

	if [ $out_fmt = ogg ] ; then
		eopt="--ogg $eopt"
	fi

	desc="($in_fmt<->$out_fmt)"

	#
	# for this we need just need just one FLAC file; --cue only works while decoding
	#
	run_flac $eopt --cuesheet=$cuesheet -o z50c.cue.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"

	# To make it easy to translate from cue point to sample numbers, the
	# file has a sample rate of 10 Hz and a cuesheet like so:
	#
	# TRACK 01, INDEX 01 : 0:00.00 -> sample 0
	# TRACK 01, INDEX 02 : 0:01.00 -> sample 10
	# TRACK 01, INDEX 03 : 0:02.00 -> sample 20
	# TRACK 02, INDEX 01 : 0:03.00 -> sample 30
	# TRACK 04, INDEX 01 : 0:04.00 -> sample 40
	#
	echo -n "testing --cue=- $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=- $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.0 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.0 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.0- $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.0- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.0- $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.1 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.1 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.1- $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.1- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.1- $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.2 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.2 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.2 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.2- $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.2- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.2- $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.4 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.4 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip20.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.4 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.4- $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.4- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip20.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.4- $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=-5.0 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=-5.0 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=-4.1 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=-4.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=-4.1 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=-3.1 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=-3.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until40.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=-3.1 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=-1.4 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=-1.4 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.until30.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=-1.4 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.0-5.0 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.0-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.0-5.0 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.1-5.0 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.1-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.1-5.0 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.2-4.1 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.2-4.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip10.until40.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.2-4.1 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	echo -n "testing --cue=1.4-2.0 $desc... "
	run_flac $dopt -o z50c.cue.$in_fmt --cue=1.4-2.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	cmp 50c.skip20.until30.$in_fmt z50c.cue.$in_fmt || die "ERROR: file mismatch for --cue=1.4-2.0 $desc"
	rm -f z50c.cue.$in_fmt
	echo OK

	rm -f z50c.cue.$out_fmt
}

test_cue raw flac
test_cue wav flac
test_cue aiff flac

if [ $has_ogg = "yes" ] ; then
	test_cue raw ogg
	test_cue wav ogg
	test_cue aiff ogg
fi

############################################################################
# test 'fixup' code that happens when a FLAC file with total_samples == 0
# in the STREAMINFO block is converted to WAVE or AIFF, requiring the
# decoder go back and fix up the chunk headers
############################################################################

echo -n "WAVE fixup test... "

echo -n "prepare... "
convert_to_wav noise || die "ERROR creating reference WAVE"

echo -n "encode... "
# the pipe from 'cat' to 'flac' does not work on cygwin because of the EOF/
# binary-mode stdin problem, so we use an undocumented option to metaflac to
# set the total sample count to 0
if [ $is_win = yes ] ; then
	run_flac $raw_eopt noise.raw -o fixup.flac || die "ERROR generating FLAC file"
	run_metaflac --set-total-samples=0 fixup.flac 2> /dev/null
else
	cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
fi

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
# the pipe from 'cat' to 'flac' does not work on cygwin because of the EOF/
# binary-mode stdin problem, so we use an undocumented option to metaflac to
# set the total sample count to 0
if [ $is_win = yes ] ; then
	run_flac $raw_eopt noise.raw -o fixup.flac || die "ERROR generating FLAC file"
	run_metaflac --set-total-samples=0 fixup.flac 2> /dev/null
else
	cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
fi

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
run_flac --verify --force --silent --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 noise.raw || die "ERROR generating FLAC file"
run_flac --decode --force --silent noise.flac || die "ERROR generating WAVE file"
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

	run_flac --force $encode_options file0.wav file1.wav file2.wav || die "ERROR"
	for n in 0 1 2 ; do
		mv file$n.$suffix file${n}x.$suffix
	done
	run_flac --force --decode file0x.$suffix file1x.$suffix file2x.$suffix || die "ERROR"
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
