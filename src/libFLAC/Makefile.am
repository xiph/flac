#
# GNU makefile
#

lib_LTLIBRARIES = libFLAC.la
if DEBUG
CFLAGS += @CFLAGS@ -DFLAC__PRECOMPUTE_PARTITION_SUMS -DFLAC__OVERFLOW_DETECT
else
CFLAGS = @CFLAGS@ -DFLAC__PRECOMPUTE_PARTITION_SUMS
endif

if FLaC__NO_ASM
else
if FLaC__CPU_IA32 
if FLaC__HAS_NASM
SUBDIRS = i386 .
libFLAC_la_LIBADD = i386/libFLAC-asm.la
endif
endif
endif

libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	cpu.c \
	crc.c \
	encoder.c \
	encoder_framing.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	memory.c \
	seek_table.c \
	stream_decoder.c
