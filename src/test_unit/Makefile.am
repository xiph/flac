CFLAGS = @CFLAGS@
INCLUDES = -I$(top_srcdir)/src/libFLAC/include

noinst_PROGRAMS = test_unit
test_unit_LDFLAGS = -lm
test_unit_LDADD = $(top_builddir)/src/libFLAC/libFLAC.la
test_unit_SOURCES = \
	bitbuffer.c \
	main.c
