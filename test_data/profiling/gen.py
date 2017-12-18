#!/usr/bin/env python3

import sys

def run(name, lines, queries):
    csvname = name + ".csv"
    inname = name + ".in"
    lines = 10**int(lines)
    queries = 10**int(queries)
    with open(csvname, "w") as f:
        print("a; 0", file=f)
        for x in range(lines):
            print(x, file=f)
    with open(inname, "w") as f:
        for i in range(0, lines, lines // queries):
            print("select a where a=%d" % i, file=f)


if __name__ == "__main__":
    run(*sys.argv[1:])
