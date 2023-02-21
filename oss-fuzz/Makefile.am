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

AM_CXXFLAGS = -std=c++11 $(LIB_FUZZING_ENGINE)
LDADD = $(flac_libs)


EXTRA_DIST = \
	fuzzer_encoder.dict \
	common.h \
	Readme.md \
	fuzzing/datasource/datasource.hpp \
	fuzzing/datasource/id.hpp \
	fuzzing/exception.hpp \
	fuzzing/memory.hpp \
	fuzzing/types.hpp

noinst_PROGRAMS =

if USE_OSSFUZZERS
noinst_PROGRAMS += fuzzer_encoder fuzzer_encoder_v2 fuzzer_decoder fuzzer_seek fuzzer_metadata fuzzer_reencoder
endif

fuzzer_encoder_SOURCES = encoder.cc
fuzzer_encoder_v2_SOURCES = encoder_v2.cc
fuzzer_decoder_SOURCES = decoder.cc
fuzzer_seek_SOURCES = seek.cc
fuzzer_metadata_SOURCES = metadata.cc
fuzzer_reencoder_SOURCES = reencoder.cc

flac_libs = \
	$(top_builddir)/src/libFLAC/libFLAC-static.la \
	$(top_builddir)/src/libFLAC++/libFLAC++-static.la \
	@OGG_LIBS@ \
	-lm

