#!/bin/sh

LD_LIBRARY_PATH=../src/libFLAC/.libs:../obj/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/test_unit:../obj/bin:$PATH

if test_unit ; then : ; else
	echo "ERROR during testgen" 1>&2
	exit 1
fi
