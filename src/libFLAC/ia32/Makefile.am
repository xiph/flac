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

SUFFIXES = .nasm .lo

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh

.nasm.o:
	$(NASM) -f $(OBJ_FORMAT) -d OBJ_FORMAT_$(OBJ_FORMAT) $< -o $@

# patch from Ben for the above rule that may be needed with new libtool:
#.nasm.lo:
#	$(LIBTOOL) --tag=CC --mode=compile \
#		$(STRIP_FPIC) $(NASM) -f $(OBJ_FORMAT) -d OBJ_FORMAT_$(OBJ_FORMAT) $<

noinst_LIBRARIES = libFLAC-asm.a
libFLAC_asm_a_SOURCES = \
	cpu_asm.nasm \
	fixed_asm.nasm \
	lpc_asm.nasm
