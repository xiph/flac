#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2012  Xiph.Org Foundation
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

LD_LIBRARY_PATH=`pwd`/../src/libFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/grabbag/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/getopt/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/replaygain_analysis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/replaygain_synthesis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/utf8/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../objs/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=`pwd`/../src/flac:$PATH

echo "Using FLAC binary :" `which flac`

date=`date "+%Y%m%dT%H%M%S"`
fname="comp${date}.flac"

last_size=0
last_k=0
for k in `seq 1 8` ; do
	flac -${k} --silent noisy-sine.wav -o ${fname}
	size=`stat --format="%s" ${fname}`
	echo "Compression level ${k}, file size ${size} bytes."
	if test $k -gt 1 ; then
		if test $last_size -lt $size ; then
			echo "Error : Compression $last_k size $last_size >= compression $k size $size."
			exit 1
			fi
		fi
	last_size=$size
	last_k=$k
	rm -f $fname
	done
