#
# GNU makefile
#

LIB_NAME  = libFLAC
INCLUDES  = -I./include -I../../include
DEBUG_CFLAGS = -DFLAC_OVERFLOW_DETECT

OBJS = \
	bitbuffer.o \
	crc.o \
	encoder.o \
	encoder_framing.o \
	file_decoder.o \
	fixed.o \
	format.o \
	lpc.o \
	md5.o \
	stream_decoder.o

include ../../build/lib.mk

# DO NOT DELETE THIS LINE -- make depend depends on it.
