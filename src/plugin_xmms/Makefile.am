#  libxmms-flac - XMMS FLAC input plugin
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

#
# GNU makefile
#

CFLAGS = @CFLAGS@ @XMMS_CFLAGS@
xmmsinputplugindir = @XMMS_INPUT_PLUGIN_DIR@
# Don't build a static library
LIBTOOL = $(top_builddir)/libtool-disable-static

xmmsinputplugin_LTLIBRARIES = libxmms-flac.la
libxmms_flac_la_SOURCES = plugin.c
libxmms_flac_la_LIBADD = $(top_builddir)/src/libFLAC/libFLAC.la @XMMS_LIBS@
#libxmms_flac_la_LIBADD = -L$(top_builddir)/src/libFLAC/.libs -lFLAC @XMMS_LIBS@
libxmms_flac_la_LDFLAGS = -module -avoid-version
