## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_LIBRARIES = libfile_utils.a

libfile_utils_a_SOURCES = file_utils.c

EXTRA_DIST = \
	Makefile.lite \
	Makefile.vc \
	file_utils.dsp

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
