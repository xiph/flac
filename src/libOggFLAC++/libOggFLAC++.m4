# Configure paths for libOggFLAC++
# "Inspired" by ogg.m4
# Caller must first run AM_PATH_LIBOGGFLAC

dnl AM_PATH_LIBOGGFLACPP([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libOggFLAC++, and define LIBOGGFLACPP_CFLAGS and LIBOGGFLACPP_LIBS
dnl
AC_DEFUN(AM_PATH_LIBOGGFLACPP,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(libOggFLACPP,[  --with-libOggFLACPP=PFX   Prefix where libOggFLAC++ is installed (optional)], libOggFLACPP_prefix="$withval", libOggFLACPP_prefix="")
AC_ARG_WITH(libOggFLACPP-libraries,[  --with-libOggFLACPP-libraries=DIR   Directory where libOggFLAC++ library is installed (optional)], libOggFLACPP_libraries="$withval", libOggFLACPP_libraries="")
AC_ARG_WITH(libOggFLACPP-includes,[  --with-libOggFLACPP-includes=DIR   Directory where libOggFLAC++ header files are installed (optional)], libOggFLACPP_includes="$withval", libOggFLACPP_includes="")
AC_ARG_ENABLE(libOggFLACPPtest, [  --disable-libOggFLACPPtest       Do not try to compile and run a test libOggFLAC++ program],, enable_libOggFLACPPtest=yes)

  if test "x$libOggFLACPP_libraries" != "x" ; then
    LIBOGGFLACPP_LIBS="-L$libOggFLACPP_libraries"
  elif test "x$libOggFLACPP_prefix" != "x" ; then
    LIBOGGFLACPP_LIBS="-L$libOggFLACPP_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    LIBOGGFLACPP_LIBS="-L$prefix/lib"
  fi

  LIBOGGFLACPP_LIBS="$LIBOGGFLACPP_LIBS -lOggFLAC++ $LIBOGGFLAC_LIBS"

  if test "x$libOggFLACPP_includes" != "x" ; then
    LIBOGGFLACPP_CFLAGS="-I$libOggFLACPP_includes"
  elif test "x$libOggFLACPP_prefix" != "x" ; then
    LIBOGGFLACPP_CFLAGS="-I$libOggFLACPP_prefix/include"
  elif test "$prefix" != "xNONE"; then
    LIBOGGFLACPP_CFLAGS="-I$prefix/include"
  fi

  LIBOGGFLACPP_CFLAGS="$LIBOGGFLACPP_CFLAGS $LIBOGGFLAC_CFLAGS"

  AC_MSG_CHECKING(for libOggFLAC++)
  no_libOggFLACPP=""


  if test "x$enable_libOggFLACPPtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $LIBOGGFLACPP_CFLAGS"
    CXXFLAGS="$CXXFLAGS $LIBOGGFLACPP_CFLAGS"
    LIBS="$LIBS $LIBOGGFLACPP_LIBS"
dnl
dnl Now check if the installed libOggFLAC++ is sufficiently new.
dnl
      rm -f conf.libOggFLAC++test
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OggFLAC++/decoder.h>

int main ()
{
  system("touch conf.libOggFLAC++test");
  return 0;
}

],, no_libOggFLACPP=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_libOggFLACPP" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.libOggFLAC++test ; then
       :
     else
       echo "*** Could not run libOggFLAC++ test program, checking why..."
       CFLAGS="$CFLAGS $LIBOGGFLACPP_CFLAGS"
       LIBS="$LIBS $LIBOGGFLACPP_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <OggFLAC++/decoder.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libOggFLAC++ or finding the wrong"
       echo "*** version of libOggFLAC++. If it is not finding libOggFLAC++, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libOggFLAC++ was incorrectly installed"
       echo "*** or that you have moved libOggFLAC++ since it was installed. In the latter case, you"
       echo "*** may want to edit the libOggFLAC++-config script: $LIBFLACPP_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     LIBOGGFLACPP_CFLAGS=""
     LIBOGGFLACPP_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(LIBOGGFLACPP_CFLAGS)
  AC_SUBST(LIBOGGFLACPP_LIBS)
  rm -f conf.libOggFLAC++test
])
