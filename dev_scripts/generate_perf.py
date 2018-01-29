#!/usr/bin/env python3.5
import sys
import argparse
from itertools import product
from pathlib import Path

from ruamel.yaml import YAML

import random


def get_tuples(cols, vals, key_len, last_val=None):
    if last_val:
        l = product(*(range(vals) for _ in range(cols - 1)), (last_val,))
    else:
        l = product(*(range(vals) for _ in range(cols)))
    if key_len == cols:
        return l
    l = list(l)
    random.shuffle(l)
    l.sort(key=lambda e: e[:key_len])
    return l

def get_csv(cols: int, vals: int, key_len: int, last_val):
    yield " ".join("c%d" % i for i in range(cols)) + "; %d" % key_len + "\n"
    for tpl in get_tuples(cols, vals, key_len, last_val):
        yield " ".join(map(str, tpl)) + "\n"

flush = 10000
pref = "insert into t values"

def get_sqlite_text(cols: int, vals: int, key_len: int, last_val):
    if key_len:
        yield "create table t (%s, primary key (%s)) without rowid;\n" \
              % (", ".join("c%d int" % i for i in range(cols)),
                 ", ".join("c%d" % i for i in range(key_len)))
    else:
        yield "create table t (%s);\n" \
              % ", ".join("c%d int" % i for i in range(cols))
    values = []
    count = 0
    for tpl in get_tuples(cols, vals, key_len, last_val):
        values.append("(%s)" % ", ".join(map(str, tpl)))
        count += 1
        if count == flush:
            count = 0
            yield "%s %s;" % (pref, ", ".join(values))
            values = []
    if values:
        yield "%s %s;" % (pref, ", ".join(values))

def make_plantydb_query(intervals):
    preds = ["c%d=[%d..%d]" % (c, l, l+r-1) for c, (l, r) in enumerate(intervals)]
    return "select * where %s" % ", ".join(preds)

def make_sqlite_query(intervals):
    preds = ["c%d >= %d AND c%d < %d" % (c, l, c, l+r) for c, (l, r) in enumerate(intervals)]
    return "select * from t where %s;" % " AND ".join(preds)

def gen_multiinterval_queries(num_queries, cols, vals=4, intervals=2):
    vals_range = range(0, vals, vals // intervals)
    with Path("in").open("w") as f:
        preds = []
        for col in range(cols):
            preds += [("c%d=%d" % (col, v)) for v in vals_range]
        q = "select * where %s\n" % ", ".join(preds)
        f.writelines(q for _ in range(num_queries))
    print("in ready")
    with Path("sql").open("w") as f:
        preds = []
        for col in range(cols):
            preds += ["(%s)" % " OR ".join([("c%d == %d" % (col, v)) for v in vals_range])]
        q = "select * from t where %s;\n" % " AND ".join(preds)
        f.writelines(q for _ in range(num_queries))
    print("sql ready")

def gen_queries(num_queries, interval_len, vals, cols):
    intervals = []
    for _ in range(num_queries):
        intervals.append([(random.randrange(vals // interval_len), interval_len)
                          for _ in range(cols)])
    with Path("in").open("w") as f:
        f.writelines([make_plantydb_query(iv) + "\n" for iv in intervals])
    print("in ready")
    with Path("sql").open("w") as f:
        f.writelines([make_sqlite_query(iv) + "\n" for iv in intervals])
    print("sql ready")

def run(vals, cols, keylen, queries, interval, last_val=None, intervals=1):
    with Path("csv").open("w") as f:
        f.writelines(get_csv(cols, vals, keylen, last_val))
    print("csv ready")
    with Path("sqlite").open("w") as f:
        f.writelines(get_sqlite_text(cols, vals, keylen, last_val))
    print("sqlite ready")
    if intervals == 1:
        gen_queries(queries, interval, vals, cols)
    else:
        assert interval == 1 and vals >= 3
        gen_multiinterval_queries(queries, cols, vals+1, intervals)

if __name__ == "__main__":
    def _run():
        with (Path(sys.argv[0]).parent / "perf_presets.yaml").open("r") as f:
            presets_config = YAML(typ="safe").load(f.read())
        preset_names = list(presets_config["presets"])
        parent_parser = argparse.ArgumentParser(add_help="data is written to $PWD")
        subparsers = parent_parser.add_subparsers(dest="cmd")
        subparsers.required = True
        parser = subparsers.add_parser("preset")
        parser.add_argument("presets", nargs="+", choices=preset_names)
        parser = subparsers.add_parser("list")
        parser = subparsers.add_parser("custom")
        parser.add_argument("--vals", type=int, required=True)
        parser.add_argument("--cols", type=int, required=True)
        parser.add_argument("--keylen", type=int, required=True)
        parser.add_argument("--queries", type=int, required=True)
        parser.add_argument("--interval", type=int, required=True)
        parser.add_argument("--intervals", type=int, default=1)
        parser.add_argument("--last_val", type=int, default=None)
        args = parent_parser.parse_args()
        if args.cmd == "list":
            print(", ".join(preset_names))
        elif args.cmd == "preset":
            for preset in args.presets:
                run(**presets_config["presets"][preset]["params"])
        elif args.cmd == "custom":
            run(**args.__dict__)

    _run()
