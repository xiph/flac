#  libFLAC++ - Free Lossless Audio Codec library
#  Copyright (C) 2002,2003  Josh Coalson
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
	libFLAC++_dynamic.dsp \
	libFLAC++_static.dsp \
	libFLAC++.m4

# see 'http://www.gnu.org/software/libtool/manual.html#Versioning' for numbering convention
libFLAC___la_LDFLAGS = -version-info 3:1:1

libFLAC___la_SOURCES = \
	file_decoder.cpp \
	file_encoder.cpp \
	metadata.cpp \
	seekable_stream_decoder.cpp \
	seekable_stream_encoder.cpp \
	stream_decoder.cpp \
	stream_encoder.cpp
