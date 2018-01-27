import subprocess
from itertools import product

import re
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
def call_planty_db(tmpdir):
    return subprocess.run("/home/ks/a/planty-db/src/debug.e {csv} < {inp} 1> {out} 2> {err}".format(
        csv=tmpdir / "csv", inp=tmpdir / "in", out=tmpdir / "out", err=tmpdir / "err"), shell=True)\
        .returncode


@pytest.mark.parametrize("test_input,key_len,intervals_reversed",
                         product(test_sets.interval_pairs, [0, 1], [True, False]))
def test_interval_pair(tmpdir, test_input, key_len, intervals_reversed):
    intervals, results, _ = test_input
    if intervals_reversed:
        intervals.reverse()
    write_queries(tmpdir, [make_query(["c"], [intervals])])
    write_csv(tmpdir, make_csv(["c"], test_sets.column, key_len=key_len))

    rc = call_planty_db(tmpdir)

    assert rc == 0
    assert [(["c"], [[r] for r in results])] == extract_results(read_out(tmpdir))

@pytest.mark.parametrize("test_input,key_len",
                         product(
                             test_sets.interval_singles + test_sets.intervals_in_relation_to_data,
                             [0, 1]))
def test_interval_single(tmpdir, test_input, key_len):
    interval, results, _ = test_input
    write_queries(tmpdir, [make_query(["c"], [[interval]])])
    write_csv(tmpdir, make_csv(["c"], test_sets.column, key_len=key_len))

    rc = call_planty_db(tmpdir)

    assert rc == 0
    assert [(["c"], [[r] for r in results])] == extract_results(read_out(tmpdir))

@pytest.mark.parametrize("case", test_sets.plan_tests)
def test_plan(tmpdir, case: test_sets.case):
    cols = ["c%d" % c for c in range(case.columns_count)]
    write_queries(tmpdir, [make_query(cols, [[x] for x in case.preds])])
    write_csv(tmpdir, make_csv(cols, case.values, case.keylen))

    rc = call_planty_db(tmpdir)

    assert rc == 0
    assert [case.result.fullscan_column] == extract_first_remaining_column(read_err(tmpdir))

@pytest.mark.parametrize("case", test_sets.OutputOrderingTests.cases)
def test_output_ordering(tmpdir, case: test_sets.OutputOrderingTests.case):
    cols = ["c%d" % c for c in range(case.columns_count)]
    write_queries(tmpdir, [make_query(cols, [[x] for x in case.preds])])
    write_csv(tmpdir, make_csv(cols, case.values, 0))

    rc = call_planty_db(tmpdir)

    assert rc == 0
    assert [(cols, [list(x) for x in case.result])] == extract_results(read_out(tmpdir))
