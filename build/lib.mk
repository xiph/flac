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
# GNU makefile fragment for building a library
#

ifeq ($(DARWIN_BUILD),yes)
CC          = cc
else
CC          = gcc
endif
NASM        = nasm
LINK        = ar cru
ifeq ($(DARWIN_BUILD),yes)
LINKD       = $(CC) -dynamiclib -flat_namespace -undefined suppress -install_name ../../obj/lib/libFLAC.dylib
#LINKD       = $(CC) -dynamiclib -flat_namespace -undefined suppress -install_name ../../obj/lib/libFLAC.1.dylib -compatibility_version 3 -current_version 3.1
else
LINKD       = ld -G
endif
LIBPATH     = ../../obj/lib
STATIC_LIB  = $(LIBPATH)/$(LIB_NAME).a
ifeq ($(DARWIN_BUILD),yes)
DYNAMIC_LIB = $(LIBPATH)/$(LIB_NAME).dylib
#DYNAMIC_LIB = $(LIBPATH)/$(LIB_NAME).1.1.1.dylib
else
DYNAMIC_LIB = $(LIBPATH)/$(LIB_NAME).so
endif

all : release

include ../../build/config.mk

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W -Winline -DFLaC__INLINE=__inline__ -DVERSION=$(VERSION) $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

debug   : $(ORDINALS_H) $(STATIC_LIB) $(DYNAMIC_LIB)
release : $(ORDINALS_H) $(STATIC_LIB) $(DYNAMIC_LIB)

$(STATIC_LIB) : $(OBJS)
	$(LINK) $@ $(OBJS) && ranlib $@

$(DYNAMIC_LIB) : $(OBJS)
ifeq ($(DARWIN_BUILD),yes)
	$(LINKD) -o $@ $(OBJS) $(LFLAGS) $(LIBS) -lc
else
	$(LINKD) -o $@ $(OBJS) $(LFLAGS) $(LIBS)
endif

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@

%.o : %.nasm
	$(NASM) -f elf -d OBJ_FORMAT_elf -i ia32/ $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(STATIC_LIB) $(DYNAMIC_LIB) $(ORDINALS_H)

.PHONY : depend
depend:
	makedepend -- $(CFLAGS) $(INCLUDES) -- *.c
