#!/bin/sh

aclocal && autoconf && autoheader && automake --foreign --include-deps --add-missing --copy
