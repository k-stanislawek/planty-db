#!/bin/bash
(
cd $(dirname $( readlink -f $0))
tests="10x2 1x4 2x2 syntax_errors semantics_errors"
for t in $tests; do
    echo $t
    if [ "$1" == "apply" ]; then
        ./debug.e $t.csv < $t.in 2> /dev/null > $t.out
    else
        ./debug.e $t.csv < $t.in 2> /dev/null | diff $t.out -
    fi
done
)
