#!/bin/bash
(
cd $(dirname $( readlink -f $0))
echo cd $PWD
tests="10x2 1x4 2x2 syntax_errors semantics_errors no_values key key2 key3"

function run() {
    suf=$1
    shift
    cmd="./debug.e $t.csv < $t.in 2> $t.err$suf > $t.out$suf"
    echo $cmd
    eval $cmd
}

for t in $tests; do
    if [ "$1" == "apply" ]; then
        run
        cat $t.out
        echo
        echo
    elif [ "$1" == "clean" ]; then
        rm -f $t.*.tmp
    else
        run .tmp
        diff $t.out $t.out.tmp
    fi
done
)
