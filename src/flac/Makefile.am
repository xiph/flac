bin_PROGRAMS = flac
CFLAGS = @CFLAGS@

flac_SOURCES = \
	decode.c \
	encode.c \
	main.c
flac_LDFLAGS = -lm
flac_LDADD = $(top_builddir)/src/libFLAC/libFLAC.la
