#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2002,2003,2004  Josh Coalson
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
PATH=../src/flac:../src/metaflac:../obj/release/bin:../obj/debug/bin:$PATH

flacfile=metaflac.flac

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"
metaflac --help 1>/dev/null 2>/dev/null || die "ERROR can't find metaflac executable"

run_flac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 flac $* 4>>test_metaflac.valgrind.log
	else
		flac $*
	fi
}

run_metaflac ()
{
	if [ x"$FLAC__VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --logfile-fd=4 metaflac $* 4>>test_metaflac.valgrind.log
	else
		metaflac $*
	fi
}

echo "Generating stream..."
if [ -f /bin/sh.exe ] ; then
	inputfile=/bin/sh.exe
else
	inputfile=/bin/sh
fi
if run_flac --verify -0 --output-name=$flacfile --force-raw-format --endian=big --sign=signed --channels=1 --bps=8 --sample-rate=44100 $inputfile ; then
	chmod +w $flacfile
else
	die "ERROR during generation"
fi

check_exit ()
{
	exit_code=$?
	[ $exit_code = 0 ] || die "ERROR, exit code = $exit_code"
}

check_flac ()
{
	run_flac --silent --test $flacfile || die "ERROR in $flacfile" 1>&2
}

check_flac

(set -x && run_metaflac --list $flacfile)
check_exit

(set -x &&
run_metaflac \
	--show-md5sum \
	--show-min-blocksize \
	--show-max-blocksize \
	--show-min-framesize \
	--show-max-framesize \
	--show-sample-rate \
	--show-channels \
	--show-bps \
	--show-total-samples \
	$flacfile
)
check_exit

(set -x && run_metaflac --preserve-modtime --add-padding=12345 $flacfile)
check_exit
check_flac

# some flavors of /bin/sh (e.g. Darwin's) won't even handle quoted spaces, so we underscore:
(set -x && run_metaflac --set-vc-field="ARTIST=The_artist_formerly_known_as_the_artist..." $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --set-vc-field="ARTIST=Chuck_Woolery" $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --set-vc-field="ARTIST=Vern" $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --set-vc-field="TITLE=He_who_smelt_it_dealt_it" $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --show-vc-vendor --show-vc-field=ARTIST $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-firstfield=ARTIST $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-field=ARTIST $flacfile)
check_exit
check_flac

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --list --block-number=0 $flacfile)
check_exit

(set -x && run_metaflac --list --block-number=1,2,999 $flacfile)
check_exit

(set -x && run_metaflac --list --block-type=VORBIS_COMMENT,PADDING $flacfile)
check_exit

(set -x && run_metaflac --list --except-block-type=SEEKTABLE,VORBIS_COMMENT $flacfile)
check_exit

(set -x && run_metaflac --add-padding=4321 $flacfile $flacfile)
check_exit
check_flac

(set -x && run_metaflac --merge-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --add-padding=0 $flacfile)
check_exit
check_flac

(set -x && run_metaflac --sort-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --add-padding=0 $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove-vc-all $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove --block-number=1,99 --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove --block-number=99 --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove --block-type=PADDING $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove --block-type=PADDING --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --add-padding=0 $flacfile $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove --except-block-type=PADDING $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove-all $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove-all --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --remove-all --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && run_metaflac --set-vc-field="f=0123456789abcdefghij" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghi" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789abcde" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghi" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && run_metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghij" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && echo "TITLE=Tittle" | run_metaflac --import-vc-from=- $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

cat > vc.txt << EOF
artist=Fartist
artist=artits
EOF
(set -x && run_metaflac --import-vc-from=vc.txt $flacfile)
check_exit
check_flac
(set -x && run_metaflac --list --block-type=VORBIS_COMMENT $flacfile)
check_exit

rm vc.txt

cs_in=cuesheets/good.000.cue
cs_out=metaflac.cue
cs_out2=metaflac2.cue
(set -x && run_metaflac --import-cuesheet-from="$cs_in" $flacfile)
check_exit
check_flac
(set -x && run_metaflac --export-cuesheet-to=$cs_out $flacfile)
check_exit
(set -x && run_metaflac --remove --block-type=CUESHEET $flacfile)
check_exit
check_flac
(set -x && run_metaflac --import-cuesheet-from=$cs_out $flacfile)
check_exit
check_flac
(set -x && run_metaflac --export-cuesheet-to=$cs_out2 $flacfile)
check_exit
echo "comparing cuesheets:"
diff $cs_out $cs_out2 || die "ERROR, cuesheets should be identical"
echo identical

rm -f $cs_out $cs_out2

(set -x && run_metaflac --add-replay-gain $flacfile)
check_exit
check_flac

echo -n "Testing FLAC file with unknown metadata... "
cp -p metaflac.flac.in $flacfile
# remove the VORBIS_COMMENT block so vendor string changes don't interfere with the comparison:
run_metaflac --remove --block-type=VORBIS_COMMENT --dont-use-padding $flacfile
cmp $flacfile metaflac.flac.ok || die "ERROR, $flacfile and metaflac.flac.ok differ"
echo OK

exit 0
