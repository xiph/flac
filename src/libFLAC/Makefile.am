#
# GNU makefile
#

lib_LTLIBRARIES = libFLAC.la
if DEBUG
CFLAGS += @CFLAGS@ -DFLAC_OVERFLOW_DETECT
else
CFLAGS = @CFLAGS@
endif

libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	crc.c \
	encoder.c \
	encoder_framing.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	seek_table.c \
	stream_decoder.c
