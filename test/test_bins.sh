#!/bin/sh

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:../obj/bin:$PATH
BINS_PATH=../../test_files/bins

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name: encode..."
	cmd="flac -V -s -fr -fb -fs 44100 -fp $bps -fc $channels $encode_options $name.bin"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	if $cmd 2>>./streams.log ; then : ; else
		echo "ERROR during encode of $name" 1>&2
		exit 1
	fi
	echo -n "decode..."
	cmd="flac -s -fb -d -fr $name.flac";
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
		for extras in '' '-p' '-e' ; do
			for blocksize in '' '-b 32' '-b 32768' '-b 65535' ; do
				for channels in 1 2 4 8 ; do
					for bps in 8 16 24 ; do
						test_file $BINS_PATH/$f $channels $bps "-$opt $extras $blocksize"
					done
				done
			done
		done
	done
done
