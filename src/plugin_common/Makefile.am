## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = include .

INCLUDES = -I$(top_srcdir)/include @ID3LIB_CFLAGS@

noinst_LIBRARIES = libplugin_common.a

noinst_HEADERS = \
	all.h \
	canonical_tag.h \
	charset.h \
	defs.h \
	dither.h \
	id3v1.h \
	id3v2.h \
	locale_hack.h \
	replaygain_synthesis.h \
	vorbiscomment.h

libplugin_common_a_SOURCES = \
	canonical_tag.c \
	charset.c \
	dither.c \
	id3v1.c \
	id3v2.c \
	replaygain_synthesis.c \
	vorbiscomment.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	README \
	plugin_common_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
