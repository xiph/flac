## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LIBRARIES = libplugin-common.a

noinst_HEADERS = \
	all.h \
	canonical_tag.h \
	charset.h \
	dither.h \
	id3v1.h \
	id3v2.h

libplugin_common_a_SOURCES = \
	canonical_tag.c \
	charset.c \
	dither.c \
	id3v1.c \
	id3v2.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	README

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
