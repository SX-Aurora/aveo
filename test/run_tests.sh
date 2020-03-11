#!/bin/bash
#
# Super simple run loop for running the tests for AVEO.
# Can be improved a lot...
#
good=0
bad=0

declare -A failed

for t in test_*; do
    echo
    echo "## running $t ##"
    echo
    #
    # run static test
    #
    if [ "${t%_static}" != "$t" ]; then
	./$t
        if [ $? -ne 0 ]; then
            bad=$((bad+1))
            failed[$t]=1
        else
            good=$((good+1))
        fi
	continue
    fi
    #
    # run ftrace test
    #
    if [ "${t%_ftrace}" != "$t" ]; then
	[ -f ftrace.out ] && rm -f ftrace.out
	env VEORUN_BIN=./aveorun-ftrace ./$t
        if [ $? -ne 0 ]; then
            bad=$((bad+1))
            failed[$t]=1
        else
            good=$((good+1))
        fi
	sleep 1
        (
	    [ -f "ftrace.out" ] || ( echo "no ftrace.out"; exit 1; )
	    /opt/nec/ve/bin/ftrace -f ftrace.out || exit 1
        )
        if [ $? -ne 0 ]; then
            bad=$((bad+1))
            failed[$t]=1
        else
            good=$((good+1))
        fi
	sleep 1
	continue
    fi
    #
    # run normal dynamic test
    #
    env VEORUN_BIN=./aveorun ./$t
    if [ $? -ne 0 ]; then
        bad=$((bad+1))
        failed[$t]=1
    else
        good=$((good+1))
    fi
    echo
done

echo
echo "============================================================"
echo "$good tests passed"
echo "$bad tests failed"

if [ $bad -gt 0 ]; then
    echo
    echo "The failed tests were:"
    echo ${failed[*]}
fi

echo "============================================================"

