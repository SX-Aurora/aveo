#!/bin/bash

for t in test_*; do
    echo
	echo "## running $t ##"
	echo
	#
	# run static test
	#
	if [ "${t%_static}" != "$t" ]; then
		./$t || exit 1
		continue
	fi
	#
	# run ftrace test
	#
	if [ "${t%_ftrace}" != "$t" ]; then
		[ -f ftrace.out ] && rm -f ftrace.out
		env VEORUN_BIN=./aveorun-ftrace ./$t || exit 1
		sleep 1
		[ -f "ftrace.out" ] || ( echo "no ftrace.out"; exit 1; )
		/opt/nec/ve/bin/ftrace -f ftrace.out || exit 1
		sleep 1
		continue
	fi
	#
	# run normal dynamic test
	#
	env VEORUN_BIN=./aveorun ./$t || exit 1
	echo
done
