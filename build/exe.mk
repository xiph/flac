#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002  Josh Coalson
#
#  This program is part of FLAC; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

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
# LINKAGE can be forced to -static or -dynamic from invocation if desired, but it defaults to -static except on OSX
ifeq ($(DARWIN_BUILD),yes)
LINKAGE     = -dynamic
else
LINKAGE     = -static
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
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W -Winline -DFLaC__INLINE=__inline__ -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

#@@@ OBJS = $(SRCS_C:%.c=%.o) $(SRCS_CC:%.cc=%.o) $(SRCS_CPP:%.cpp=%.o) $(SRCS_NASM:%.nasm=%.o)
#@@@ OBJS = $(SRCS_C:%.c=%.$(BUILD).o) $(SRCS_CC:%.cc=%.$(BUILD).o) $(SRCS_CPP:%.cpp=%.$(BUILD).o) $(SRCS_NASM:%.nasm=%.$(BUILD).o)
DEBUG_OBJS = $(SRCS_C:%.c=%.debug.o) $(SRCS_CC:%.cc=%.debug.o) $(SRCS_CPP:%.cpp=%.debug.o) $(SRCS_NASM:%.nasm=%.debug.o)
RELEASE_OBJS = $(SRCS_C:%.c=%.release.o) $(SRCS_CC:%.cc=%.release.o) $(SRCS_CPP:%.cpp=%.release.o) $(SRCS_NASM:%.nasm=%.release.o)

debug   : $(ORDINALS_H) $(DEBUG_PROGRAM)
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
	-rm -f *.o $(OBJPATH)/*/bin/$(PROGRAM_NAME)

.PHONY : depend
depend:
	makedepend -fMakefile.lite -- $(CFLAGS) $(INCLUDES) -- *.c *.cc *.cpp
