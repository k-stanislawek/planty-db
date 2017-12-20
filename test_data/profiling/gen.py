#!/usr/bin/env python3

import sys
import os

def run(directory, lines, queries, times=1):
    if not os.path.isdir(directory):
        os.mkdir(directory)
    csvname = os.path.join(directory, "0.csv")
    inname = os.path.join(directory, "0.in")
    lines = 10**int(lines)
    queries = 10**int(queries)
    times = int(times)
    with open(csvname, "w") as f:
        print("a; 0", file=f)
        for x in range(lines):
            print(x, file=f)
    with open(inname, "w") as f:
        for t in range(times):
            for i in range(0, lines, lines // queries):
                print("select a where a=%d" % i, file=f)


if __name__ == "__main__":
    run(*sys.argv[1:])
