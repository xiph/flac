#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003  Josh Coalson
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

if FLaC__EXHAUSTIVE_TESTS
FLAC__EXHAUSTIVE_TESTS = yes
else
FLAC__EXHAUSTIVE_TESTS = no
endif

if FLaC__VALGRIND_TESTING
FLAC__VALGRIND = yes
else
FLAC__VALGRIND = no
endif

TESTS_ENVIRONMENT = FLAC__EXHAUSTIVE_TESTS=$(FLAC__EXHAUSTIVE_TESTS) FLAC__VALGRIND=$(FLAC__VALGRIND)

SUBDIRS = cuesheets

if FLaC__HAS_OGG
OGG_TESTS = \
	./test_libOggFLAC.sh \
	./test_libOggFLAC++.sh
endif

TESTS = \
	./test_libFLAC.sh \
	./test_libFLAC++.sh \
	$(OGG_TESTS) \
	./test_flac.sh \
	./test_metaflac.sh \
	./test_grabbag.sh \
	./test_streams.sh \
	./test_bins.sh

EXTRA_DIST = \
	Makefile.lite \
	cuesheet.ok \
	test_libFLAC.sh \
	test_libFLAC++.sh \
	test_libOggFLAC.sh \
	test_libOggFLAC++.sh \
	test_flac.sh \
	test_metaflac.sh \
	test_grabbag.sh \
	test_streams.sh \
	test_bins.sh

CLEANFILES = \
	$(wildcard *.raw) \
	$(wildcard *.flac) \
	$(wildcard *.ogg) \
	$(wildcard *.cmp) \
	$(wildcard *.aiff) \
	$(wildcard *.wav) \
	$(wildcard ../../test_files/bins/*.raw) \
	$(wildcard ../../test_files/bins/*.flac) \
	$(wildcard ../../test_files/bins/*.cmp) \
	$(wildcard *.diff) \
	$(wildcard *.log) \
	$(wildcard *.cue) \
	core
