## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include/share

noinst_LTLIBRARIES = libgain_analysis.la

libgain_analysis_la_SOURCES = gain_analysis.c

EXTRA_DIST = \
	Makefile.lite \
	gain_analysis_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
