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
noinst_PROGRAMS += fuzzer_encoder fuzzer_encoder_v2 fuzzer_decoder fuzzer_seek fuzzer_metadata fuzzer_reencoder fuzzer_tool_flac
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

fuzzer_reencoder_SOURCES = fuzzer_reencoder.cc
fuzzer_reencoder_CXXFLAGS = $(AM_CXXFLAGS) $(LIB_FUZZING_ENGINE)
fuzzer_reencoder_LDFLAGS = $(AM_LDFLAGS)
fuzzer_reencoder_LDADD = $(flac_libs)

fuzzer_tool_flac_SOURCES = ${flac_SOURCES} fuzzer_tool_flac.c
fuzzer_tool_flac_CFLAGS = $(LIB_FUZZING_ENGINE)
fuzzer_tool_flac_LDFLAGS = $(AM_LDFLAGS)
fuzzer_tool_flac_LDADD =  \
        $(top_builddir)/src/share/utf8/libutf8.la \
        $(top_builddir)/src/share/grabbag/libgrabbag.la \
        $(top_builddir)/src/share/getopt/libgetopt.la \
        $(top_builddir)/src/share/replaygain_analysis/libreplaygain_analysis.la \
        $(top_builddir)/src/share/replaygain_synthesis/libreplaygain_synthesis.la \
        $(top_builddir)/src/libFLAC/libFLAC.la \
        @LTLIBICONV@ \
        -lm

flac_clibs = \
	$(top_builddir)/src/libFLAC/libFLAC-static.la \
	@OGG_LIBS@ \
	-lm

flac_libs = \
	$(top_builddir)/src/libFLAC/libFLAC-static.la \
	$(top_builddir)/src/libFLAC++/libFLAC++-static.la \
	@OGG_LIBS@ \
	-lm


flac_SOURCES = \
        ${top_builddir}/src/flac/analyze.c \
        ${top_builddir}/src/flac/decode.c \
        ${top_builddir}/src/flac/encode.c \
        ${top_builddir}/src/flac/foreign_metadata.c \
        ${top_builddir}/src/flac/local_string_utils.c \
        ${top_builddir}/src/flac/utils.c \
        ${top_builddir}/src/flac/vorbiscomment.c \
        ${top_builddir}/src/flac/analyze.h \
        ${top_builddir}/src/flac/decode.h \
        ${top_builddir}/src/flac/encode.h \
        ${top_builddir}/src/flac/foreign_metadata.h \
        ${top_builddir}/src/flac/local_string_utils.h \
        ${top_builddir}/src/flac/utils.h \
        ${top_builddir}/src/flac/vorbiscomment.h
