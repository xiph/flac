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

aclocal && libtoolize && autoconf && autoheader && $hacks && automake --foreign --include-deps --add-missing --copy
