#
# GNU makefile
#

SUFFIXES = .nasm
.nasm.o:
	$(NASM) -f elf -d ELF -i i386/ $< -o $@

lib_LTLIBRARIES = libFLAC.la
if DEBUG
CFLAGS += @CFLAGS@ -DFLAC__PRECOMPUTE_PARTITION_SUMS -DFLAC__OVERFLOW_DETECT
else
CFLAGS = @CFLAGS@ -DFLAC__PRECOMPUTE_PARTITION_SUMS
endif

if FLaC__CPU_IA32 
if FLaC__HAS_NASM
libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	crc.c \
	cpu.c \
	encoder.c \
	encoder_framing.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	seek_table.c \
	stream_decoder.c \
	i386/cpu_asm.nasm \
	i386/fixed_asm.nasm \
	i386/lpc_asm.nasm
else
libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	crc.c \
	cpu.c \
	encoder.c \
	encoder_framing.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	seek_table.c \
	stream_decoder.c
endif
else
libFLAC_la_SOURCES = \
	bitbuffer.c \
	bitmath.c \
	crc.c \
	cpu.c \
	encoder.c \
	encoder_framing.c \
	file_decoder.c \
	fixed.c \
	format.c \
	lpc.c \
	md5.c \
	seek_table.c \
	stream_decoder.c
endif
