#!/bin/sh

gettextize --intl && aclocal && autoconf && autoheader && automake --foreign --include-deps --add-missing --copy
