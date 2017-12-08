#!/bin/bash +x

./debug.e 0.csv < 0.in 2> /dev/null | diff 0.out -
./debug.e 1.csv < 1.in
./debug.e 2.csv
