#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001  Josh Coalson
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

CC          = gcc
NASM        = nasm
# LINKAGE can be forced to -static or -dynamic from invocation if desired, but it defaults to -static
LINKAGE     = -static
LINK        = gcc $(LINKAGE)
BINPATH     = ../../obj/bin
LIBPATH     = ../../obj/lib
PROGRAM     = $(BINPATH)/$(PROGRAM_NAME)

all : release

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W -DVERSION="1.0devel" $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -ffast-math -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W -DVERSION="1.0devel" $(DEFINES) $(INCLUDES)

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
	$(NASM) -f elf -d ELF -i ia32/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(PROGRAM)

.PHONY : depend
depend:
	makedepend -- $(CFLAGS) $(INCLUDES) -- *.c
