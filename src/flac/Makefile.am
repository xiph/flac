#  flac - Command-line FLAC encoder/decoder
#  Copyright (C) 2000,2001  Josh Coalson
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
CFLAGS = @CFLAGS@

flac_SOURCES = \
	analyze.c \
	decode.c \
	encode.c \
	file.c \
	main.c
flac_LDFLAGS = -lm
flac_LDADD = $(top_builddir)/src/libFLAC/libFLAC.la
