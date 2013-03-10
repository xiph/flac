## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = libgrabbag.la

libgrabbag_la_SOURCES = \
	alloc.c \
	cuesheet.c \
	file.c \
	picture.c \
	replaygain.c \
	seektable.c

EXTRA_DIST = \
	Makefile.lite \
	grabbag_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
