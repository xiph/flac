#
# GNU makefile fragment for building a library
#

CC          = gcc
LINK        = ar cru
LINKD       = ld -G
LIBPATH     = ../../obj/lib
STATIC_LIB  = $(LIBPATH)/$(LIB_NAME).a
DYNAMIC_LIB = $(LIBPATH)/$(LIB_NAME).so

all : release

debug   : CFLAGS = -g -O0 -DDEBUG $(DEBUG_CFLAGS) -Wall -W $(DEFINES) $(INCLUDES)
release : CFLAGS = -O3 -fomit-frame-pointer -funroll-loops -ffast-math -finline-functions -DNDEBUG $(RELEASE_CFLAGS) -Wall -W $(DEFINES) $(INCLUDES)

LFLAGS  = -L$(LIBPATH)

debug   : $(STATIC_LIB) $(DYNAMIC_LIB)
release : $(STATIC_LIB) $(DYNAMIC_LIB)

$(STATIC_LIB) : $(OBJS)
	$(LINK) $@ $(OBJS)

$(DYNAMIC_LIB) : $(OBJS)
	$(LINKD) -o $@ $(OBJS) $(LFLAGS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.i : %.c
	$(CC) $(CFLAGS) -E $< -o $@

.PHONY : clean
clean :
	-rm -f $(OBJS) $(STATIC_LIB) $(DYNAMIC_LIB)

.PHONY : depend
depend:
	makedepend -- $(CFLAGS) $(INCLUDES) -- *.c
