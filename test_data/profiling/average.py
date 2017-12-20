#!/usr/bin/env python3

import sys
from collections import defaultdict

def run(perf_file):
    vals = defaultdict(list)
    with open(perf_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            row = line.split(" ")
            assert row[0] == "perf:"
            vals[int(row[1])].append(int(row[2]))
    s = 0
    k = sorted(vals)
    if not k:
        return
    print("first", min(*vals[k[0]]))
    print("min", min(*map(min, vals.values())))

if __name__ == "__main__":
    run(*sys.argv[1:])

