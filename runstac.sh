#!/bin/sh
core=core.stc
bin="./stac"
if [ $# -eq 0 ]; then
	args="-"
else
	args="$*"
fi

cat $core $args | $bin
