#!/bin/sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../src/libFLAC/.libs
export LD_LIBRARY_PATH

if ../src/test_streams/test_streams ; then : ; else
	echo "ERROR during test_streams" 1>&2
	exit 1
fi

FLAC=../src/flac/flac

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo "### ENCODE $name ########################################" >> ./encode.log
	echo -n "$name: encode..."
	if $FLAC -s -fb -fs 44100 -fp $bps -fc $channels -0 -l 8 -m -e $encode_options $name.raw $name.flac 2>>./encode.log ; then : ; else
		echo "ERROR during encode of $name" 1>&2
		exit 1
	fi
	echo "### DECODE $name ########################################" >> ./decode.log
	echo -n "decode..."
	if $FLAC -s -fb -d -fr $name.flac $name.cmp 2>>./decode.log ; then : ; else
		echo "ERROR during decode of $name" 1>&2
		exit 1
	fi
	echo -n "compare..."
	if cmp $name.raw $name.cmp ; then : ; else
		echo "ERROR during compare of $name" 1>&2
		exit 1
	fi
	echo OK
}

echo "Testing small files..."
test_file test01 1 16
test_file test02 2 16
test_file test03 1 16
test_file test04 2 16

echo "Testing 8-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd8-$b 1 8 "-q 15"
done

echo "Testing 16-bit full-scale deflection streams..."
for b in 01 02 03 04 05 06 07 ; do
	test_file fsd16-$b 1 16 "-q 15"
done

echo "Testing sine wave streams..."
for b in 01 02 03 04 05 ; do
	test_file sine-$b 1 16 ""
done

echo "Testing some frame header variations..."
test_file sine-02 1 16 "--lax -b 16"
test_file sine-02 1 16 "--lax -b 65535"
test_file sine-02 1 16 "-b 16"
test_file sine-02 1 16 "-b 65535"
test_file sine-02 1 16 "--lax -fs 9"
test_file sine-02 1 16 "--lax -fs 90"
test_file sine-02 1 16 "--lax -fs 90000"
test_file sine-02 1 16 "-fs 9"
test_file sine-02 1 16 "-fs 90"
test_file sine-02 1 16 "-fs 90000"
