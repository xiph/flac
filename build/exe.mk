#
# GNU makefile fragment for building an executable
#

CC          = gcc
NASM        = nasm
# LINKAGE can be forced to -static or -dynamic from invocation if desired, but it defaults to -static
LINKAGE     = -static
LINK        = gcc $(LINKAGE)
BINPATH     = ../../obj/bin
LIBPATH     = ../../obj/lib
PROGRAM     = $(BINPATH)/$(PROGRAM_NAME)

all : release

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -ffast-math -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

debug   : $(PROGRAM)
release : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(LINK) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@

%.o : %.nasm
	$(NASM) -f elf -d ELF -i i386/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(PROGRAM)

.PHONY : depend
depend:
	makedepend -- $(CFLAGS) $(INCLUDES) -- *.c
