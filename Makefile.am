#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
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
# automake provides the following useful targets:
#
# all: build all programs and libraries using the current
# configuration (set by configure)
#
# check: build and run all self-tests
#
# clean: remove everything except what's required to build everything
#
# distclean: remove everything except what goes in the distribution
#

SUBDIRS = doc include man src test build flac.pbproj obj

DISTCLEANFILES = libtool-disable-static

EXTRA_DIST = \
	COPYING.FDL \
	COPYING.GPL \
	COPYING.LGPL \
	FLAC.dsw \
	Makefile.lite \
	Makefile.vc \
	autogen.sh \
	depcomp \
	ltmain.sh \
	strip_fPIC.sh

AUTOMAKE_OPTIONS = foreign
