## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_builddir) -I$(srcdir)/include -I$(top_srcdir)/include -I$(top_srcdir)/include/share

noinst_LTLIBRARIES = libreplaygain_analysis.la

libreplaygain_analysis_la_SOURCES = replaygain_analysis.c

EXTRA_DIST = \
	Makefile.lite \
	replaygain_analysis_static.vcproj

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
