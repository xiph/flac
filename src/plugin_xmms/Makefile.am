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

EXTRA_DIST = \
	Makefile.lite

noinst_HEADERS = \
	charset.h \
	configure.h \
	wrap_id3.h

CFLAGS = @CFLAGS@  @ ID3LIB_CFLAGS@  @ XMMS_CFLAGS@ 
INCLUDES = -I$(top_srcdir)/src
xmmsinputplugindir = @ XMMS_INPUT_PLUGIN_DIR@ 

# Don't build a static library
LIBTOOL = $(top_builddir)/libtool-disable-static

xmmsinputplugin_LTLIBRARIES = libxmms-flac.la

plugin_sources = charset.c configure.c plugin.c wrap_id3.c

if FLaC__HAS_ID3LIB
libxmms_flac_la_SOURCES = $(plugin_sources)
else
libxmms_flac_la_SOURCES = $(plugin_sources)
endif

# work around the bug in libtool where its relinking fails with a different DESTDIR
# for libtool bug info see:
#   http://mail.gnu.org/pipermail/bug-libtool/2002-February/003018.html
#   http://mail.gnu.org/pipermail/libtool/2002-April/006244.html
#   http://mail.gnu.org/pipermail/libtool/2002-April/006250.html
# for fix info see:
#   http://lists.freshrpms.net/pipermail/rpm-list/2002-April/000746.html
# the workaround is the extra '-L$(top_builddir)/src/libFLAC/.libs'
libxmms_flac_la_LIBADD = $(top_builddir)/src/plugin_common/libplugin_common.a $(top_builddir)/src/libFLAC/libFLAC.la -L$(top_builddir)/src/libFLAC/.libs @ XMMS_LIBS@  @ ID3LIB_LIBS@ 
libxmms_flac_la_LDFLAGS = -module -avoid-version
