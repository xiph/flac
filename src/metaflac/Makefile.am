#  metaflac - Command-line FLAC metadata editor
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

bin_PROGRAMS = metaflac
CFLAGS = @CFLAGS@

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc

metaflac_SOURCES = \
	main.c
metaflac_LDFLAGS = 

metaflac_LDADD = $(top_builddir)/src/libFLAC/libFLAC.la @SHARE_LIBS@ -lm
