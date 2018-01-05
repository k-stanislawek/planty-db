from operator import mul
from itertools import product
import random

def multiply(where_limits):
    res = 1
    for l in where_limits:
        res *= l
    return res

def make_column_names(n):
    return ["c%d" % c for c in range(n)]

def make_query(column_names, where_intervals):
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

def make_csv(column_names, key_len, values_list):
    return " ".join(column_names) + "; %d\n" % key_len + sjoin2(values_list)

def test_f_fullscan(n_columns: int, last_value: int, key_len: int):
    column_names = make_column_names(n_columns)
    values_list = list(product(*(range(last_value) for _ in range(n_columns))))
    random.seed(13)
    random.shuffle(values_list)
    if key_len:
        values_list.sort(key=lambda value: value[:key_len])
    where_limits = tuple(last_value // 2 + (x % last_value) for x in range(n_columns))

    result_values = []
    for value in values_list:
        if all(l < r for l, r in zip(value, where_limits)):
            result_values.append(value)

    where_intervals = ["(..%d)" % l for l in where_limits]

    csv = make_csv(column_names, key_len, values_list)
    inp = make_query(column_names, where_intervals)
    outp = sjoin2([column_names] + result_values)
    return csv, inp, outp
