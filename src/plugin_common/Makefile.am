#  plugin_common - Routines common to several plugins
#  Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LTLIBRARIES = libplugin_common.la

noinst_HEADERS = \
	all.h \
	charset.h \
	defs.h \
	dither.h \
	locale_hack.h \
	tags.h

libplugin_common_la_SOURCES = \
	charset.c \
	dither.c \
	tags.c

EXTRA_DIST = \
	Makefile.lite \
	README \
	plugin_common_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
