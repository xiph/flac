# Configure paths for libFLAC++
# "Inspired" by ogg.m4

dnl AM_PATH_LIBFLACPP([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libFLAC++, and define LIBFLACPP_CFLAGS and LIBFLACPP_LIBS
dnl
AC_DEFUN(AM_PATH_LIBFLACPP,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(libFLAC++,[  --with-libFLAC++=PFX   Prefix where libFLAC++ is installed (optional)], libFLACPP_prefix="$withval", libFLACPP_prefix="")
AC_ARG_WITH(libFLAC++-libraries,[  --with-libFLAC++-libraries=DIR   Directory where libFLAC++ library is installed (optional)], libFLACPP_libraries="$withval", libFLACPP_libraries="")
AC_ARG_WITH(libFLAC++-includes,[  --with-libFLAC++-includes=DIR   Directory where libFLAC++ header files are installed (optional)], libFLACPP_includes="$withval", libFLACPP_includes="")
AC_ARG_ENABLE(libFLAC++test, [  --disable-libFLAC++test       Do not try to compile and run a test libFLAC++ program],, enable_libFLACPPtest=yes)

  if test "x$libFLACPP_libraries" != "x" ; then
    LIBFLACPP_LIBS="-L$libFLACPP_libraries"
  elif test "x$libFLACPP_prefix" != "x" ; then
    LIBFLACPP_LIBS="-L$libFLACPP_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    LIBFLACPP_LIBS="-L$prefix/lib"
  fi

  LIBFLACPP_LIBS="$LIBFLACPP_LIBS -lFLAC++"

  if test "x$libFLACPP_includes" != "x" ; then
    LIBFLACPP_CFLAGS="-I$libFLACPP_includes"
  elif test "x$libFLACPP_prefix" != "x" ; then
    LIBFLACPP_CFLAGS="-I$libFLACPP_prefix/include"
  elif test "$prefix" != "xNONE"; then
    LIBFLACPP_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for libFLAC++)
  no_libFLACPP=""


  if test "x$enable_libFLACPPtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $LIBFLACPP_CFLAGS"
    LIBS="$LIBS $LIBFLACPP_LIBS"
dnl
dnl Now check if the installed libFLAC++ is sufficiently new.
dnl
      rm -f conf.libFLAC++test
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC++/decoder.h>

int main ()
{
  system("touch conf.libFLAC++test");
  return 0;
}

],, no_libFLACPP=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_libFLACPP" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.libFLAC++test ; then
       :
     else
       echo "*** Could not run libFLAC++ test program, checking why..."
       CFLAGS="$CFLAGS $LIBFLACPP_CFLAGS"
       LIBS="$LIBS $LIBFLACPP_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <FLAC++/decoder.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libFLAC++ or finding the wrong"
       echo "*** version of libFLAC++. If it is not finding libFLAC++, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libFLAC++ was incorrectly installed"
       echo "*** or that you have moved libFLAC++ since it was installed. In the latter case, you"
       echo "*** may want to edit the libFLAC++-config script: $LIBFLACPP_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     LIBFLACPP_CFLAGS=""
     LIBFLACPP_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(LIBFLACPP_CFLAGS)
  AC_SUBST(LIBFLACPP_LIBS)
  rm -f conf.libFLAC++test
])
