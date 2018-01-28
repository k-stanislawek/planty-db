#!/usr/bin/env python3.5
from itertools import product
from pathlib import Path

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


# - wylaczyc logowanie
# - odpalic callgrinda i kcachegrinda na moim kodzie na profiling.e
# Y50: ~/a/planty-db/build6 (master)$ time (sqlite3 db < sql 1> /dev/null 2> /dev/null)
# - sprawdzic koszt stdout/stderr
# time ( sqlite3 -init sqlite < sql 1> /dev/null 2> /dev/null )
# time ( ../src/release.e csv 1> /dev/null 2> /dev/null < in )

# do sprawdzenia:
# - sqlite na dysku
# - duzo range'ow w kolumnie
# - duzo kolumn

# dla wyniku dlugosci 1, w ktorym mozna uzyc rangescanu:
# - sqlite: 20s ładowanie, 20s zapytania
# - plantydb: 2s ladowanie, 12s zapytania

# dla wyniku 1% z tablicy wielkosci 10^6, w ktorym byl fullscan na 10% danych:
# - plantydb: 1m 7s
# - sqlite: 5m

# fullscan 1 kolumna: 10x szybszy plantydb

#     vals = 2
#     cols = 20
#     queries = 1000
#     keylen = 20
#     interval = 1
# plantydb: 2s, sqlite: 60s, sqlite na dysku: podobnie!

# z dwoma intervalami w te same kolumnie dostaiemy w dupe.


# dla
#     vals = 3
#     cols = 13
#     keylen = 4
#     queries = 100
#     interval = 1
#     last_val = 1
#     intervals = 2
# sqlite masakruie 5xkrotnie.
# dla keylen=0 iest prawie remis.
# dla keylen=13 wygrywamy 10x, pewnie na skutek rowid.
# czemu przegrywamy dla keylen=4? nie wiadomo.
# dla cols=1 i keylen=0 wygrywamy.

# mieszanie koncowki nic nie daie

# konkluzia: sqlite pewnie robi binsearch po wielu kolumnach naraz
# a moze iakies cacheowanie/nauka?

def run():
    vals = 3
    cols = 10
    keylen = 1
    queries = 500
    interval = 1
    last_val = 1
    intervals = 1
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
    run()
