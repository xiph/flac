#!/bin/sh

aclocal && autoconf && automake --foreign --include-deps --add-missing --copy
