#!/usr/bin/env python3
import subprocess
from itertools import product

import re
import sys
from io import StringIO

import pytest
from tests import test_sets


def make_output(column_names, output):
    f = StringIO()
    f.write(" ".join(column_names) + "\n")
    for t in output:
        if isinstance(t, int):
            t = [t]
        f.write(" ".join(map(str, t)) + "\n")
    return f.getvalue()


def write_outputs(tmpdir, outputs):
    with open(tmpdir / "out") as f:
        for n, output in enumerate(outputs, start=1):
            f.write("query number: %d\n" % n)
            f.write(output)


def make_csv(column_names, values, key_len):
    f = StringIO()
    f.write(" ".join(column_names) + "; %d\n" % key_len)
    for v in values:
        if isinstance(v, int):
            v = [v]
        f.write(" ".join(map(str, v)) + "\n")
    return f.getvalue()


def write_csv(tmpdir, csv):
    (tmpdir / "csv").write(csv)


def write_queries(tmpdir, queries):
    with (tmpdir / "in").open("w") as f:
        for query in queries:
            f.write(query + "\n")


def make_query(columns, intervalslist):
    preds = []
    for column, intervals in zip(columns, intervalslist):
        for interval in intervals:
            preds.append(column + "=" + interval)
    return "select %s where %s" % (", ".join(columns), ", ".join(preds))


fullscan_col_re = re.compile("^plan: Range scan result: \(first_remaining_column=(\d+)*")


def extract_first_remaining_column(lines):
    results = []
    for line in lines:
        if line.startswith("plan:"):
            res = fullscan_col_re.match(line)
            if not res:
                continue
            results.append(int(res.group(1)))
    return results


def read_err(tmpdir):
    with (tmpdir / "err").open("r") as f:
        return list(f)


def extract_results(lines):
    results = []
    current_result = []
    current_header = []
    for l in lines:
        l = l.rstrip()
        if not l:
            continue
        if l.startswith("query number:"):
            if current_header:
                results.append((current_header, current_result))
                current_header = []
                current_result = []
            continue
        t = l.split(" ")
        if not current_header:
            current_header = t
        else:
            current_result.append(list(map(int, t)))
    if current_header:
        results.append((current_header, current_result))
    return results


def read_out(tmpdir):
    with (tmpdir / "out").open("r") as f:
        return list(f)


def test_make_query():
    intervals = ["[1..2)", "[3..3]"]

    res = make_query(["c"], [intervals])

    assert res == "select c where c=[1..2), c=[3..3]"


def test_make_csv():
    cols = ["a"]
    vals = [1, 2]

    res = make_csv(cols, vals, 1).rstrip().split("\n")

    assert res == ["a; 1", "1", "2"]


def test_extract_first_remaining_column():
    s = ["dupa", "plan: dupa",
         "plan: Range scan result: (first_remaining_column=3, rows=<1..2>)",
         "plan: Range scan result: (first_remaining_column=4, rows=<1..2>)"]

    res = extract_first_remaining_column(s)

    assert res == [3, 4]


def test_extract_results():
    lines = [
        "",
        "   ",
        "query number:",
        "a b c  \n",
        "1 1 1  ",
        "2 2 2  ",
        "query number: 1",
        "a",
        "4",
        "query number: 2",
        "q"
    ]

    results = extract_results(lines)

    assert results == [
        (["a", "b", "c"], [[1, 1, 1], [2, 2, 2]]),
        (["a"], [[4]]),
        (["q"], [])
    ]


# noinspection PyShadowingNames
def call_planty_db(tmpdir, plantydb):
    return subprocess.run("{plantydb} {csv} < {inp} 1> {out} 2> {err}".format(
        plantydb=plantydb, csv=tmpdir / "csv", inp=tmpdir / "in", out=tmpdir / "out",
        err=tmpdir / "err"), shell=True).returncode


@pytest.mark.parametrize("test_input,key_len,intervals_reversed",
                         product(test_sets.interval_pairs, [0, 1], [True, False]))
def test_interval_pair(tmpdir, plantydb, test_input, key_len, intervals_reversed):
    intervals, results, _ = test_input
    if intervals_reversed:
        intervals.reverse()
    write_queries(tmpdir, [make_query(["c"], [intervals])])
    write_csv(tmpdir, make_csv(["c"], test_sets.column, key_len=key_len))

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(["c"], [[r] for r in results])] == extract_results(read_out(tmpdir))


@pytest.mark.parametrize("test_input,key_len",
                         product(
                             test_sets.interval_singles + test_sets.intervals_in_relation_to_data,
                             [0, 1]))
def test_interval_single(tmpdir, plantydb, test_input, key_len):
    interval, results, _ = test_input
    write_queries(tmpdir, [make_query(["c"], [[interval]])])
    write_csv(tmpdir, make_csv(["c"], test_sets.column, key_len=key_len))

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(["c"], [[r] for r in results])] == extract_results(read_out(tmpdir))


@pytest.mark.parametrize("case", test_sets.plan_tests)
def test_plan(tmpdir, plantydb, case: test_sets.case):
    cols = ["c%d" % c for c in range(case.columns_count)]
    write_queries(tmpdir, [make_query(cols, [[x] for x in case.preds])])
    write_csv(tmpdir, make_csv(cols, case.values, case.keylen))

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [case.result.fullscan_column] == extract_first_remaining_column(read_err(tmpdir))


@pytest.mark.parametrize("case", test_sets.OutputOrderingTests.cases)
def test_output_ordering(tmpdir, plantydb, case: test_sets.OutputOrderingTests.case):
    cols = ["c%d" % c for c in range(case.columns_count)]
    write_queries(tmpdir, [make_query(cols, [[x] for x in case.preds])])
    write_csv(tmpdir, make_csv(cols, case.values, 0))

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(cols, [list(x) for x in case.result])] == extract_results(read_out(tmpdir))


@pytest.mark.parametrize("key_len", [0, 1])
def test_empty_csv(tmpdir, plantydb, key_len):
    cols = ["c0", "c1"]
    write_csv(tmpdir, make_csv(cols, [], key_len))
    write_queries(tmpdir, [make_query(cols, [["0", "[1..)"], ["(..1]", "2"]])])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(cols, [])] == extract_results(read_out(tmpdir))


@pytest.mark.parametrize("key_len", [0, 1])
def test_empty_csv(tmpdir, plantydb, key_len):
    cols = ["c0", "c1"]
    write_csv(tmpdir, make_csv(cols, [], key_len))
    write_queries(tmpdir, [make_query(cols, [["0", "[1..)"], ["(..1]", "2"]])])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(cols, [])] == extract_results(read_out(tmpdir))


def test_no_where(tmpdir, plantydb):
    cols = ["a"]
    write_csv(tmpdir, make_csv(cols, [[1]], 0))
    write_queries(tmpdir, ["select a"])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(cols, [[1]])] == extract_results(read_out(tmpdir))


def test_select(tmpdir, plantydb):
    cols = ["a", "b", "c"]
    write_csv(tmpdir, make_csv(cols, [[1, 2, 3]], 0))
    write_queries(tmpdir, [
        "select a",
        "select b, a",
        "select *",
        "select *, a"
    ])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert [(["a"], [[1]]),
            (["b", "a"], [[2, 1]]),
            (["a", "b", "c"], [[1, 2, 3]]),
            (["a", "b", "c", "a"], [[1, 2, 3, 1]])
            ] == extract_results(read_out(tmpdir))


def test_wrong_column(tmpdir, plantydb):
    cols = ["a"]
    write_csv(tmpdir, make_csv(cols, [], 0))
    write_queries(tmpdir, [make_query("select b", [])])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert ["query error: unknown column name: s"] == [l.rstrip() for l in read_out(tmpdir)]


def test_unsorted_key_column(tmpdir, plantydb):
    cols = ["a", "b"]
    write_csv(tmpdir, make_csv(cols, [[1, 2], [1, 1]], 2))
    write_queries(tmpdir, [])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 26
    assert ["table error: key of row 1 is lesser than previous row"] == \
           [l.rstrip() for l in read_out(tmpdir)]


def test_syntax_errors(tmpdir, plantydb):
    cols = ["a", "b"]
    write_csv(tmpdir, make_csv(cols, [[1, 2]], 2))
    write_queries(tmpdir, [
        "",
        "select a, where b=1",
        "select a,",
        "select a b where a=1",
        "select a b",
        "select where a=1",
        "select",
        "select a where a=1,",
        "select a where a=1 b=1",
        "select a where",
        "select a where a=",
        "select a where a=b",
        "select a where =[..1)",
        "select a where a=[1..1",
        "select a where a=1..1]",
        "select a where a=[1-1]",
        "select a where a=[a..2]",
        "select   a     where        a=1     "
    ])

    rc = call_planty_db(tmpdir, plantydb)

    assert rc == 0
    assert ['query error: no select at the beginning',
            'query error: unknown column name: where',
            'query error: no comma after select list',
            "query error: something else than 'where' after select list: b",
            "query error: something else than 'where' after select list: b",
            'query error: unknown column name: where',
            'query error: select list empty',
            'query error: no comma after where list',
            "query error: there's something after 'where': b=1",
            'query error: where list empty',
            'query error: Error during converting to integer:',
            'query error: Error during converting to integer: b',
            'query error: unknown column name:',
            'query error: bad range close:  [1..1',
            'query error: bad range open:  1..1]',
            'query error: Error during converting to integer: [1-1]',
            'query error: Error during converting to integer: a',
            'query number: 1', 'a', '1'] == \
           [l.rstrip() for l in read_out(tmpdir)]

if __name__ == "__main__":
    exit(pytest.main([sys.argv[0]]))