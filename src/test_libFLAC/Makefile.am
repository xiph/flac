#  test_libFLAC - Unit tester for libFLAC
#  Copyright (C) 2000,2001,2002  Josh Coalson
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

CFLAGS = @CFLAGS@
INCLUDES = -I$(top_srcdir)/src/libFLAC/include

noinst_PROGRAMS = test_libFLAC
test_libFLAC = $(top_builddir)/src/libFLAC/libFLAC.la -lm
test_libFLAC_SOURCES = \
	bitbuffer.c \
	decoders.c \
	encoders.c \
	file_utils.c \
	main.c \
	metadata.c \
	metadata_manip.c \
	metadata_object.c \
	metadata_utils.c \
	bitbuffer.h \
	decoders.h \
	encoders.h \
	file_utils.h \
	metadata.h \
	metadata_utils.h
