## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LIBRARIES = libreplaygain.a

libreplaygain_a_SOURCES = replaygain.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	replaygain.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
