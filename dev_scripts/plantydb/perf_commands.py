import logging
import os
from collections import defaultdict

from pathlib import Path
from typing import Optional

def gentest(testname: Optional[Path], lines, queries, times=1, key_len=0):
    if not testname:
        testname = Path("gentest.{}.{}.{}.{}".format(lines, queries, times, key_len))
    testname.mkdir(exist_ok=True)
    csvname = testname / "csv"
    inname = testname / "in"
    logging.info("  testname: %s", testname)
    lines = 10**int(lines)
    queries = 10**int(queries)
    times = int(times)
    with csvname.open("w") as f:
        f.write("a; %d\n" % key_len)
        for x in range(lines):
            f.write("%d\n" % x)
    with inname.open("w") as f:
        for _ in range(times):
            for i in range(0, lines, lines // queries):
                f.write("select a where a=%d\n" % i)

def accumulate_performance_data(inp_perf_file: Path, outp_summary_file: Path):
    vals = defaultdict(list)
    with inp_perf_file.open("r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            row = line.split(" ")
            assert row[0] == "perf:"
            vals[int(row[1])].append(int(row[2]))
    with outp_summary_file.open("w") as f:
        if vals:
            k = sorted(vals)
            f.write("first %d\n" % min(vals[k[0]]))
            f.write("min %d\n" % min(map(min, vals.values())))

def read_perf_data(p: Path):
    if not p.exists:
        return {}
    m = {}
    with open(str(p), "r") as f:
        for line in f:
            if not line:
                continue
            k, v = line.split(" ")
            m[k] = int(v)
    return m

def diff_perf_mins(fnew: Path, fprev: Path):
    new = read_perf_data(fnew)
    prev = read_perf_data(fprev)
    for k in sorted(set(new).union(prev)):
        print("{}: {} -> {}".format(k, prev.get(k), new.get(k)))

