#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2011-2014  Xiph.Org Foundation
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under different licenses.  The codec libraries
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

AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libutf8.la

libutf8_la_SOURCES = charset.c charset.h iconvert.c utf8.c

EXTRA_DIST = \
	Makefile.lite \
	charmaps.h \
	makemap.c \
	charset_test.c \
	charsetmap.h \
	iconvert.h \
	utf8_static.vcproj \
	utf8_static.vcxproj \
	utf8_static.vcxproj.filters

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
