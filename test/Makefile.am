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

if FLaC__EXHAUSTIVE_TESTS
FLAC__EXHAUSTIVE_TESTS = yes
else
FLAC__EXHAUSTIVE_TESTS = no
endif

if FLaC__VALGRIND_TESTING
FLAC__VALGRIND = yes
else
FLAC__VALGRIND = no
endif

TESTS_ENVIRONMENT = FLAC__EXHAUSTIVE_TESTS=$(FLAC__EXHAUSTIVE_TESTS) FLAC__VALGRIND=$(FLAC__VALGRIND)

SUBDIRS = cuesheets

if FLaC__HAS_OGG
OGG_TESTS = \
	./test_libOggFLAC.sh \
	./test_libOggFLAC++.sh
endif

TESTS = \
	./test_libFLAC.sh \
	./test_libFLAC++.sh \
	$(OGG_TESTS) \
	./test_flac.sh \
	./test_metaflac.sh \
	./test_grabbag.sh \
	./test_streams.sh \
	./test_bins.sh

EXTRA_DIST = \
	Makefile.lite \
	cuesheet.ok \
	metaflac.flac.in \
	metaflac.flac.ok \
	test_libFLAC.sh \
	test_libFLAC++.sh \
	test_libOggFLAC.sh \
	test_libOggFLAC++.sh \
	test_flac.sh \
	test_metaflac.sh \
	test_grabbag.sh \
	test_streams.sh \
	test_bins.sh

CLEANFILES = \
	$(wildcard *.raw) \
	$(wildcard *.flac) \
	$(wildcard *.ogg) \
	$(wildcard *.cmp) \
	$(wildcard *.aiff) \
	$(wildcard *.wav) \
	$(wildcard *.diff) \
	$(wildcard *.log) \
	$(wildcard *.cue) \
	core
