## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include/share

noinst_LIBRARIES = libgain_analysis.a

libgain_analysis_a_SOURCES = gain_analysis.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	gain_analysis_static.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
