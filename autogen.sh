#!/bin/sh

# 'hacks' is the place to put some commands you may need.  There are at
# least two that seem to be necessary in some situations:
#
# 1. Some (newer?) versions automake --add-missing --copy do not copy
#    in ltmain.sh, maybe because this is now supposed to be done by
#    libtoolize.
# 2. FLAC uses iconv but not gettext.  iconv requires config.rpath which
#    is supplied by gettext, which is copied in by gettextize.  But we
#    can't run gettextize since we do not fulfill all it's requirements
#    (since we don't use it).
#
# If both these apply try:
#
#hacks="cp /usr/share/libtool/ltmain.sh . && cp /usr/share/gettext/config.rpath ."
#
# Otherwise, this is the no-op:
hacks=true
#

aclocal && autoconf && autoheader && $hacks && automake --foreign --include-deps --add-missing --copy
