#  flac - Command-line FLAC encoder/decoder
#  Copyright (C) 2000,2001,2002,2003  Josh Coalson
#
#  This program is free software; you can redistribute it and/or
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

bin_PROGRAMS = flac
CFLAGS = @CFLAGS@ @OGG_CFLAGS@

if FLaC__HAS_OGG
NEED_OGGFLAC_LIB = $(top_builddir)/src/libOggFLAC/libOggFLAC.la
endif

EXTRA_DIST = \
	Makefile.lite \
	flac.dsp

flac_SOURCES = \
	analyze.c \
	decode.c \
	encode.c \
	main.c \
	string.c \
	utils.c \
	vorbiscomment.c \
	analyze.h \
	decode.h \
	encode.h \
	string.h \
	utils.h \
	vorbiscomment.h

flac_LDADD = \
	$(NEED_OGGFLAC_LIB) \
	$(top_builddir)/src/share/grabbag/libgrabbag.la \
	$(top_builddir)/src/share/getopt/libgetopt.a \
	$(top_builddir)/src/share/replaygain_analysis/libreplaygain_analysis.la \
	$(top_builddir)/src/share/replaygain_synthesis/libreplaygain_synthesis.la \
	$(top_builddir)/src/share/utf8/libutf8.la \
	$(top_builddir)/src/libFLAC/libFLAC.la \
	@OGG_LIBS@ \
	@LIBICONV@ \
	-lm
