#!/bin/bash

kbscan="1 4 16 32 64 96 128 256 512 768"
mbscan="1 2 4 8 16 32 64 256 1024"


printf "%8s   %7s   %7s\n" "buff kB" "send MB/s" "recv MB/s"
for s in $kbscan; do
    bw=`./bandwidth $((s * 1024)) 2>&1 | grep "bw=" | sed -e 's,^.*bw=,,' -e 's,\..*$,,'`
    printf "%8d   %7.0f   %7.0f\n" $s $bw
done
for s in $mbscan; do
    bw=`./bandwidth $((s * 1024 * 1024)) 2>&1 | grep "bw=" | sed -e 's,^.*bw=,,' -e 's,\..*$,,'`
    printf "%8d   %7.0f   %7.0f\n" $((s*1024)) $bw
done
