## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LIBRARIES = libgrabbag.a

libgrabbag_a_SOURCES = \
	cuesheet.c \
	file.c \
	replaygain.c \
	seektable.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	grabbag_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
