## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include/share

noinst_LTLIBRARIES = libreplaygain_analysis.la

libreplaygain_analysis_la_SOURCES = replaygain_analysis.c

EXTRA_DIST = \
	Makefile.lite \
	replaygain_analysis_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
