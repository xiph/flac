#  libFLAC - Free Lossless Audio Codec library
#  Copyright (C) 2001,2002,2003,2004  Josh Coalson
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  - Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  - Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  - Neither the name of the Xiph.org Foundation nor the names of its
#  contributors may be used to endorse or promote products derived from
#  this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
if FLaC__CPU_PPC
ARCH_SUBDIRS = ppc
libFLAC_la_LIBADD = ppc/libFLAC-asm.la
LOCAL_EXTRA_LDFLAGS = "-Wl,-read_only_relocs,warning"
endif
endif

SUBDIRS = $(ARCH_SUBDIRS) include .

m4datadir = $(datadir)/aclocal
m4data_DATA = libFLAC.m4

EXTRA_DIST = \
	Makefile.lite \
	libFLAC_dynamic.dsp \
	libFLAC_static.dsp \
	libFLAC.m4

# see 'http://www.gnu.org/software/libtool/manual.html#SEC35' for numbering convention
libFLAC_la_LDFLAGS = -version-info 6:1:0 -lm $(LOCAL_EXTRA_LDFLAGS)
libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	cpu.c \
	crc.c \
	file_decoder.c \
	file_encoder.c \
	fixed.c \
	float.c \
	format.c \
	lpc.c \
	md5.c \
	memory.c \
	metadata_iterators.c \
	metadata_object.c \
	seekable_stream_decoder.c \
	seekable_stream_encoder.c \
	stream_decoder.c \
	stream_encoder.c \
	stream_encoder_framing.c
