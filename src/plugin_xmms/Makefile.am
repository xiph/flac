#
# GNU makefile
#

CFLAGS = @CFLAGS@ @XMMS_CFLAGS@
xmmsinputplugindir = @XMMS_INPUT_PLUGIN_DIR@
# Don't build a static library
LIBTOOL = $(top_builddir)/libtool-disable-static

xmmsinputplugin_LTLIBRARIES = libxmms-flac.la
libxmms_flac_la_SOURCES = plugin.c
libxmms_flac_la_LIBADD = -L$(top_builddir)/src/libFLAC/.libs -lFLAC @XMMS_LIBS@
libxmms_flac_la_LDFLAGS = -module -avoid-version
