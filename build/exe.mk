#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

#
# GNU makefile fragment for building an executable
#

include $(topdir)/build/config.mk

ifeq ($(DARWIN_BUILD),yes)
CC          = cc
CCC         = c++
else
CC          = gcc
CCC         = g++
endif
NASM        = nasm
# override to -dynamic on OSX
ifeq ($(DARWIN_BUILD),yes)
LINKAGE     = -dynamic
endif
LINK        = $(CC) $(LINKAGE)
OBJPATH     = $(topdir)/obj
BINPATH     = $(OBJPATH)/$(BUILD)/bin
LIBPATH     = $(OBJPATH)/$(BUILD)/lib
DEBUG_BINPATH   = $(OBJPATH)/debug/bin
DEBUG_LIBPATH   = $(OBJPATH)/debug/lib
RELEASE_BINPATH = $(OBJPATH)/release/bin
RELEASE_LIBPATH = $(OBJPATH)/release/lib
PROGRAM         = $(BINPATH)/$(PROGRAM_NAME)
DEBUG_PROGRAM   = $(DEBUG_BINPATH)/$(PROGRAM_NAME)
RELEASE_PROGRAM = $(RELEASE_BINPATH)/$(PROGRAM_NAME)

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
valgrind: CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -DFLAC__VALGRIND_TESTING -Wall -W -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W -Winline -DFLaC__INLINE=__inline__ -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

DEBUG_OBJS = $(SRCS_C:%.c=%.debug.o) $(SRCS_CC:%.cc=%.debug.o) $(SRCS_CPP:%.cpp=%.debug.o) $(SRCS_NASM:%.nasm=%.debug.o)
RELEASE_OBJS = $(SRCS_C:%.c=%.release.o) $(SRCS_CC:%.cc=%.release.o) $(SRCS_CPP:%.cpp=%.release.o) $(SRCS_NASM:%.nasm=%.release.o)

debug   : $(ORDINALS_H) $(DEBUG_PROGRAM)
valgrind: $(ORDINALS_H) $(DEBUG_PROGRAM)
release : $(ORDINALS_H) $(RELEASE_PROGRAM)

$(DEBUG_PROGRAM) : $(DEBUG_OBJS)
	$(LINK) -o $@ $(DEBUG_OBJS) $(LFLAGS) $(LIBS)

$(RELEASE_PROGRAM) : $(RELEASE_OBJS)
	$(LINK) -o $@ $(RELEASE_OBJS) $(LFLAGS) $(LIBS)

%.debug.o %.release.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.debug.o %.release.o : %.cc
	$(CCC) $(CFLAGS) -c $< -o $@
%.debug.o %.release.o : %.cpp
	$(CCC) $(CFLAGS) -c $< -o $@
%.debug.i %.release.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@
%.debug.i %.release.i : %.cc
	$(CCC) $(CFLAGS) -E $< -o $@
%.debug.i %.release.i : %.cpp
	$(CCC) $(CFLAGS) -E $< -o $@

%.debug.o %.release.o : %.nasm
	$(NASM) -f elf -d OBJ_FORMAT_elf -i ia32/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(DEBUG_OBJS) $(RELEASE_OBJS) $(OBJPATH)/*/bin/$(PROGRAM_NAME)

.PHONY : depend
depend:
	makedepend -fMakefile.lite -- $(CFLAGS) $(INCLUDES) -- *.c *.cc *.cpp
