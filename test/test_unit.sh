#!/bin/sh

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../obj/lib
export LD_LIBRARY_PATH

if ../obj/bin/test_unit ; then : ; else
	echo "ERROR during testgen" 1>&2
	exit 1
fi
