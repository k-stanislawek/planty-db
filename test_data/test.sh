#!/bin/bash
(
cd $(dirname $( readlink -f $0))
tests="10x2 1x4 2x2 syntax_errors semantics_errors no_values"
for t in $tests; do
    echo $t
    if [ "$1" == "apply" ]; then
        ./debug.e $t.csv < $t.in 2> $t.err.full > $t.out
        grep "^log:" $t.err.full > $t.err.logs
    elif [ "$1" == "clean" ]; then
        rm -f $t.*.tmp
    else
        ./debug.e $t.csv < $t.in 2> $t.err.full.tmp | diff $t.out -
        grep "^log:" $t.err.full.tmp > $t.err.logs.tmp
        diff $t.err.logs $t.err.logs.tmp
    fi
done
)
