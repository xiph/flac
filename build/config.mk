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
# debug/release selection
#

DEFAULT_BUILD = release

debug    : BUILD = debug
valgrind : BUILD = debug
release  : BUILD = release

debug    : LINKAGE = -static
valgrind : LINKAGE = -dynamic
release  : LINKAGE = -static

all default: $(DEFAULT_BUILD)

#
# GNU makefile fragment for emulating stuff normally done by configure
#

VERSION=\"1.0.4\"

ORDINALS_H = $(topdir)/include/FLAC/ordinals.h

$(ORDINALS_H): $(ORDINALS_H).in
	sed \
		-e "s/@FLaC__SIZE16@/short/g" \
		-e "s/@FLaC__SIZE32@/int/g" \
		-e "s/@FLaC__SIZE64@/long long/g" \
		-e "s/@FLaC__USIZE16@/unsigned short/g" \
		-e "s/@FLaC__USIZE32@/unsigned int/g" \
		-e "s/@FLaC__USIZE64@/unsigned long long/g" \
		$< > $@
