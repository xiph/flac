#!/bin/sh

# 'hacks' is the place to put some commands you may need.  There is one
# that seems to be necessary in some situations:
#
#  * FLAC uses iconv but not gettext.  iconv requires config.rpath which
#    is supplied by gettext, which is copied in by gettextize.  But we
#    can't run gettextize since we do not fulfill all it's requirements
#    (because we don't use it).
#
# If the default doesn't work, try:
#
#hacks="cp /usr/share/gettext/config.rpath ."
#
# Otherwise, this is the no-op:
hacks=true
#
# Also watchout, if you replace ltmain.sh, there is a bug in some
# versions of libtool (or maybe autoconf) on some platforms where the
# configure-generated libtool does not have $SED defined.  See also:
#
#   http://lists.gnu.org/archive/html/libtool/2003-11/msg00131.html
#

aclocal && libtoolize && autoconf && autoheader && $hacks && automake --foreign --include-deps --add-missing --copy
