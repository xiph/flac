## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include -I$(top_srcdir)/include -I$(top_srcdir)/include/share

noinst_LIBRARIES = libgetopt.a

libgetopt_a_SOURCES = getopt.c getopt1.c

EXTRA_DIST = \
	Makefile.lite \
	getopt_static.dsp \
	getopt_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
