#  libFLAC++ - Free Lossless Audio Codec library
#  Copyright (C) 2002  Josh Coalson
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the
#  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA  02111-1307, USA.

lib_LTLIBRARIES = libFLAC++.la
CXXFLAGS = @CXXFLAGS@

m4datadir = $(datadir)/aclocal
m4data_DATA = libFLAC++.m4

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	libFLAC++.m4

# see 'http://www.gnu.org/software/libtool/manual.html#Versioning' for numbering convention
libFLAC___la_LDFLAGS = -version-info 1:1:0

libFLAC___la_SOURCES = \
	file_decoder.cc \
	file_encoder.cc \
	metadata.cc \
	seekable_stream_decoder.cc \
	seekable_stream_encoder.cc \
	stream_decoder.cc \
	stream_encoder.cc
