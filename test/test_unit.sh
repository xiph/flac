#!/bin/sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../src/libFLAC/.libs
export LD_LIBRARY_PATH

if ../src/test_unit/test_unit ; then : ; else
	echo "ERROR during testgen" 1>&2
	exit 1
fi
