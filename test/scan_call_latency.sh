#!/bin/bash

export LC_ALL=C

niters="1 5 10 50 100 500 1000 5000 10000 50000 100000 200000 300000"

cat <<EOI
VEO call latency test
=====================

Test 1: submit N calls, then wait for N results
Test 2: submit one call and wait for its result, N times
Test 3: submit N calls and wait only for last result
Test 4: submit N synchronous calls

EOI

printf "%8s   %7s   %7s   %7s   %7s\n" "#calls" "Test 1" "Test 2" "Test 3" "Test 4"
for s in $niters; do
    DATA=$(./call_latency $s 2>&1 | egrep "/call$" | sed -e 's/^.*\, //' -e 's,[0]*us/call,,')
    printf "%8d   %7.2f   %7.2f   %7.2f   %7.2f\n" $s $DATA
done
