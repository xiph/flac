## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include/share

noinst_LIBRARIES = libgetopt.a

libgetopt_a_SOURCES = getopt.c getopt1.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	getopt.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
