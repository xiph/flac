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

#
# debug/release selection
#

DEFAULT_BUILD = release

debug    : BUILD = debug
valgrind : BUILD = debug
release  : BUILD = release

debug    : LINKAGE = -static
valgrind : LINKAGE = -dynamic
release  : LINKAGE = -static

all default: $(DEFAULT_BUILD)

#
# GNU makefile fragment for emulating stuff normally done by configure
#

VERSION=\"1.1.1\"

CONFIG_CFLAGS=-DHAVE_INTTYPES_H
