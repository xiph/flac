#  libFLAC - Free Lossless Audio Codec library
#  Copyright (C) 2001  Josh Coalson
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

#
# GNU makefile
#

lib_LTLIBRARIES = libFLAC.la
if DEBUG
DEBUGCFLAGS = -DFLAC__OVERFLOW_DETECT
endif
CFLAGS = @CFLAGS@ $(DEBUGCFLAGS)

if FLaC__NO_ASM
else
if FLaC__CPU_IA32
if FLaC__HAS_NASM
ARCH_SUBDIRS = ia32
libFLAC_la_LIBADD = ia32/libFLAC-asm.la
endif
endif
endif

SUBDIRS = $(ARCH_SUBDIRS) include .

libFLAC_la_LDFLAGS = -version-info 2:0:1

libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	cpu.c \
	crc.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	memory.c \
	seek_table.c \
	seekable_stream_decoder.c \
	stream_decoder.c \
	stream_encoder.c \
	stream_encoder_framing.c \
	utility.c
