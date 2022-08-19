#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2019-2022  Xiph.Org Foundation
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

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/oss-fuzz

AM_CXXFLAGS = -std=c++11

EXTRA_DIST = \
	fuzzer_encoder.dict \
	fuzzer_common.h \
	Readme.md \
	fuzzing/datasource/datasource.hpp \
	fuzzing/datasource/id.hpp \
	fuzzing/exception.hpp \
	fuzzing/memory.hpp \
	fuzzing/types.hpp

noinst_PROGRAMS =

if USE_OSSFUZZERS
noinst_PROGRAMS += fuzzer_encoder fuzzer_encoder_v2 fuzzer_decoder fuzzer_seek fuzzer_metadata
endif

fuzzer_encoder_SOURCES = fuzzer_encoder.cc
fuzzer_encoder_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_encoder_LDFLAGS = $(AM_LDFLAGS)
fuzzer_encoder_LDADD = $(flac_libs)

fuzzer_encoder_v2_SOURCES = fuzzer_encoder_v2.cc
fuzzer_encoder_v2_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_encoder_v2_LDFLAGS = $(AM_LDFLAGS)
fuzzer_encoder_v2_LDADD = $(flac_libs)

fuzzer_decoder_SOURCES = fuzzer_decoder.cc
fuzzer_decoder_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_decoder_LDFLAGS = $(AM_LDFLAGS)
fuzzer_decoder_LDADD = $(flac_libs)

fuzzer_seek_SOURCES = fuzzer_seek.cc
fuzzer_seek_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_seek_LDFLAGS = $(AM_LDFLAGS)
fuzzer_seek_LDADD = $(flac_libs)

fuzzer_metadata_SOURCES = fuzzer_metadata.cc
fuzzer_metadata_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_metadata_LDFLAGS = $(AM_LDFLAGS)
fuzzer_metadata_LDADD = $(flac_libs)


flac_libs = \
	$(top_builddir)/src/libFLAC/libFLAC-static.la \
	$(top_builddir)/src/libFLAC++/libFLAC++-static.la \
	@OGG_LIBS@ \
	-lm

