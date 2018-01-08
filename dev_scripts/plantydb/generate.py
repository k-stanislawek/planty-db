import logging
from typing import List, Tuple
from itertools import product
import operator
import random
import functools

from pathlib import Path


def multiply(where_limits):
    return functools.reduce(operator.mul, where_limits, 1)

def make_column_names(n) -> Tuple[str]:
    # noinspection PyTypeChecker
    return tuple(("c%d" % c for c in range(n)))

def make_query(column_names: Tuple[str], where_intervals):
    if not column_names:
        return "select"
    query = "select "
    query += ", ".join(column_names)
    where_clauses = ["%s=%s" % (name, interval)
                     for (name, interval) in zip(column_names, where_intervals)]
    if where_clauses:
        query += " where " + ", ".join(where_clauses)
    return query

def sjoin(seq, sep=" "):
    return sep.join(map(str, seq))

def sjoin2(seq, sep1="\n", sep2=" "):
    return sep1.join(map(lambda x: sjoin(x, sep2), seq))

def make_csv(column_names: List[str], key_len: int, values_list: List[Tuple[int]]):
    return " ".join(column_names) + "; %d\n" % key_len + sjoin2(values_list)

def generate_fullscan_case_output(values_list, where_limits):
    return [v for v in values_list if all(l <= r for l, r in zip(v, where_limits))]

def generate_fullscan_case_where_limits(n_columns: int, max_value: int):
    return tuple((max_value // 2 + x) % max_value for x in range(n_columns))

def generate_fullscan_case_table_values(n_columns: int, last_value: int, key_len: int):
    values_list = list(product(*(range(last_value) for _ in range(n_columns))))
    random.seed(13)
    random.shuffle(values_list)
    if key_len:
        values_list.sort(key=lambda v: v[:key_len])
    return values_list

def write_queries(f, queries):
    f.writelines(queries)
    f.write("\n")

def write_output(f, column_names, output):
    f.write("\n")
    f.write(" ".join(column_names) + "\n")
    for t in output:
        f.write(" ".join(map(str, t)) + "\n")

def write_csv(f, column_names: Tuple[str], values: List[Tuple[int]], key_len: int):
    f.write(" ".join(column_names) + "; %d\n" % key_len)
    for v in values:
        f.write(" ".join(map(str, v)) + "\n")

def write_test(testname: Path, column_names: Tuple[str], values: List[Tuple[int]], output: List[Tuple[int]],
               queries: List[str], key_len: int):
    with (testname / "in").open("w") as f:
        write_queries(f, queries)
    with (testname / "out").open("w") as f:
        write_output(f, column_names, output)
    with (testname / "csv").open("w") as f:
        write_csv(f, column_names, values, key_len)

def gen_fullscan_testname(*args):
    return Path(".".join(map(str, ["fullscan", *args])))

def generate_fullscan_case(testname: Path, n_columns: int, max_value: int, key_len: int):
    if not testname:
        testname = gen_fullscan_testname(n_columns, max_value, key_len)
        logging.info("  testname: %s", testname)
    testname.mkdir(exist_ok=True)
    values_list = generate_fullscan_case_table_values(n_columns, max_value, key_len)
    where_limits = generate_fullscan_case_where_limits(n_columns, max_value)
    output = generate_fullscan_case_output(values_list, where_limits)
    column_names = make_column_names(n_columns)
    query = make_query(column_names, ["(..%d]" % l for l in where_limits])
    write_test(testname, column_names, values_list, output, [query], key_len)
