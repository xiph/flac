#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2002  Josh Coalson
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
PATH=../src/flac:../src/metaflac:../obj/bin:$PATH

flacfile=metaflac.flac

flac --help 1>/dev/null 2>/dev/null || (echo "ERROR can't find flac executable" 1>&2 && exit 1)
if [ $? != 0 ] ; then exit 1 ; fi

metaflac --help 1>/dev/null 2>/dev/null || (echo "ERROR can't find metaflac executable" 1>&2 && exit 1)
if [ $? != 0 ] ; then exit 1 ; fi

echo "Generating stream..."
if flac --verify -0 --output-name=$flacfile --force-raw-input --endian=big --channels=1 --bps=8 --sample-rate=44100 /bin/sh ; then
	chmod +w $flacfile
else
	echo "ERROR during generation" 1>&2
	exit 1
fi

check_exit ()
{
	exit_code=$?
	if [ $exit_code != 0 ] ; then
		echo "ERROR, exit code = $exit_code" 1>&2
		exit 1
	fi
}

check_flac ()
{
	if flac --silent --test $flacfile ; then : ; else
		echo "ERROR in $flacfile" 1>&2
		exit 1
	fi
}

check_flac

(set -x && metaflac --list $flacfile)
check_exit

(set -x &&
metaflac \
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

(set -x && metaflac --preserve-modtime --add-padding=12345 $flacfile)
check_exit
check_flac

# some flavors of /bin/sh (e.g. Darwin's) won't handle even quoted spaces, so we underscore:
(set -x && metaflac --set-vc-field="ARTIST=The_artist_formerly_known_as_the_artist..." $flacfile)
check_exit
check_flac

(set -x && metaflac --set-vc-field="ARTIST=Chuck_Woolery" $flacfile)
check_exit
check_flac

(set -x && metaflac --set-vc-field="ARTIST=Vern" $flacfile)
check_exit
check_flac

(set -x && metaflac --set-vc-field="TITLE=He_who_smelt_it_dealt_it" $flacfile)
check_exit
check_flac

(set -x && metaflac --show-vc-vendor --show-vc-field=ARTIST $flacfile)
check_exit

(set -x && metaflac --remove-vc-firstfield=ARTIST $flacfile)
check_exit
check_flac

(set -x && metaflac --remove-vc-field=ARTIST $flacfile)
check_exit
check_flac

(set -x && metaflac --list --block-number=0 $flacfile)
check_exit

(set -x && metaflac --list --block-number=1,2,999 $flacfile)
check_exit

(set -x && metaflac --list --block-type=VORBIS_COMMENT,PADDING $flacfile)
check_exit

(set -x && metaflac --list --except-block-type=SEEKTABLE,VORBIS_COMMENT $flacfile)
check_exit

(set -x && metaflac --add-padding=4321 $flacfile $flacfile)
check_exit
check_flac

(set -x && metaflac --merge-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --add-padding=0 $flacfile)
check_exit
check_flac

(set -x && metaflac --sort-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --add-padding=0 $flacfile)
check_exit
check_flac

(set -x && metaflac --remove-vc-all $flacfile)
check_exit
check_flac

(set -x && metaflac --remove --block-number=1,99 --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --remove --block-number=99 --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --remove --block-type=PADDING $flacfile)
check_exit
check_flac

(set -x && metaflac --remove --block-type=PADDING --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --add-padding=0 $flacfile $flacfile)
check_exit
check_flac

(set -x && metaflac --remove --except-block-type=PADDING $flacfile)
check_exit
check_flac

(set -x && metaflac --remove-all $flacfile)
check_exit
check_flac

(set -x && metaflac --remove-all --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --remove-all --dont-use-padding $flacfile)
check_exit
check_flac

(set -x && metaflac --set-vc-field="f=0123456789abcdefghij" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghi" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789abcde" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghi" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

(set -x && metaflac --remove-vc-all --set-vc-field="f=0123456789abcdefghij" $flacfile)
check_exit
check_flac
(set -x && metaflac --list --except-block-type=STREAMINFO $flacfile)
check_exit

exit 0
