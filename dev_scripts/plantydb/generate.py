import logging
from typing import List, Tuple, Union, Sequence
from itertools import product, zip_longest
import operator
import random
import functools

from pathlib import Path


def multiply(where_limits):
    return functools.reduce(operator.mul, where_limits, 1)


def make_column_names(n) -> Tuple[str]:
    # noinspection PyTypeChecker
    return tuple(("c%d" % c for c in range(n)))


def make_query(column_names: Sequence[str], where_intervals: Sequence[List[str]]):
    assert len(column_names) == len(where_intervals)
    if not column_names:
        return "select"
    query = "select "
    query += ", ".join(column_names)
    where_clauses = []
    for name, intervals in zip(column_names, where_intervals):
        where_clauses += ["%s=%s" % (name, interval) for interval in intervals]
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
    for q in queries:
        f.write("%s\n" % q)


def write_output(f, column_names, outputs: List[Union[List[int], List[Tuple[int]]]]):
    for n, output in enumerate(outputs, start=1):
        f.write("query number: %d\n" % n)
        f.write(" ".join(column_names) + "\n")
        for t in output:
            if isinstance(t, int):
                t = [t]
            f.write(" ".join(map(str, t)) + "\n")


def write_csv(f, column_names: Sequence[str], values: List[Tuple[int]], key_len: int):
    f.write(" ".join(column_names) + "; %d\n" % key_len)
    for v in values:
        if isinstance(v, int):
            v = [v]
        f.write(" ".join(map(str, v)) + "\n")


class PlanFile:
    _range_scan = "plan: Range scan for column: {c} rows: <{l}..{r}>"
    _full_scan = "plan: Range scan result: (first_remaining_column={c}, rows=<{l}..{r}>)"
    _res1 = "plan: Full scan result: <{l}..{r}>"
    _res2 = "plan: Full scan result: {{{n} rows}}"

    def __init__(self):
        self._lines1 = []
        self._lines2 = []
        self._lines3 = []

    def rangescan(self, c, l, r):
        self._lines1.append(self._range_scan.format(c=c, l=l, r=r))
        return self

    def fullscan(self, c, l, r):
        self._lines2.append(self._full_scan.format(c=c, l=l, r=r))
        return self

    def res1(self, l, r):
        self._lines3.append(self._res1.format(l=l, r=r))
        return self

    def res2(self, n):
        self._lines3.append(self._res2.format(n=n))
        return self

    @property
    def lines(self):
        return self._lines1 + self._lines2 + self._lines3

    def __str__(self):
        return "\n".join(self.lines) + "\n"

    def write(self, p: Path):
        p.write_text(str(self))


known_plans = {
    # edge case when all columns are range-scanned
    # note: last column predicate can be non-exact
    "multi.1-0.0-1.2": PlanFile().rangescan(0, 0, 19).rangescan(1, 0, 4)
                                 .fullscan(2, 0, 1).res1(0, 1),
    # column fullscaned because of non-exact predicate
    "multi.0-1.1-0.2": PlanFile().rangescan(0, 0, 19).fullscan(1, 0, 7).res2(2),
    # column fullscaned because of being non-key column
    "multi.1-0.0-1.1": PlanFile().rangescan(0, 0, 19).fullscan(1, 0, 4).res2(2),
    # 100% full-scan because of no key
    "multi.1-1.0-0.0": PlanFile().fullscan(0, 0, 15).res2(1),
}


def write_test(testname: Path, column_names: Sequence[str], values: Union[List[int], List[Tuple[int]]],
               queries_with_outputs: List[Tuple[str, Union[List[int], List[Tuple[int]]]]], key_len: int):
    with (testname / "in").open("w") as f:
        write_queries(f, [q for q, _ in queries_with_outputs])
    with (testname / "out").open("w") as f:
        write_output(f, column_names, [o for _, o in queries_with_outputs])
    with (testname / "csv").open("w") as f:
        write_csv(f, column_names, values, key_len)
    if str(testname) in known_plans:
        known_plans[str(testname)].write(testname / "plan")


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
    query = make_query(column_names, [["(..%d]" % l] for l in where_limits])
    write_test(testname, column_names, values_list, [(query, output)], key_len)


def generate_interval_case(testname: Path, key_len: int):
    if not testname:
        testname = Path("interval.%d" % key_len)
    testname.mkdir(exist_ok=True)
    values = [x for x in range(2, 10)]
    qf = "select c where c=%s"
    queries = [(qf % "[3..)", list(range(3, 10))),
               (qf % "(..7]", list(range(2, 8))),
               (qf % "[3..7)", [3, 4, 5, 6]),
               (qf % "(3..7]", [4, 5, 6, 7]),
               (qf % "(..)", values),
               (qf % "[3..3]", [3]),
               (qf % "[3..3)", []),
               # over edges
               (qf % "[1..11]", values),
               (qf % "[..11]", values),
               (qf % "[1..]", values),
               (qf % "[1..5]", [2, 3, 4, 5]),
               (qf % "[7..11]", [7, 8, 9])]
    write_test(testname, ("c",), values, queries, key_len)

def generate_multicolumn_case(testname: Path,
                              exact_ranges: List[int],
                              nonexact_ranges: List[int],
                              key_len: int):
    if not testname:
        testname = Path("multi.%s.%s.%d" % ("-".join(map(str, exact_ranges)),
                                            "-".join(map(str, nonexact_ranges)),
                                            key_len))
    testname.mkdir(exist_ok=True)
    column_values = []
    column_output = []
    column_intervals = []
    for exact, nonexact in zip_longest(exact_ranges, nonexact_ranges, fillvalue=0):
        column_values.append(list(range(2 * exact + 3 * nonexact + 2)))
        output_exact = list(range(0, exact * 2, 2))
        intervals_exact = ["[{0}..{0}]".format(v)
                           for v in output_exact]
        output_nonexact = [v + i
                           for v in range(exact * 2, exact * 2 + nonexact * 3, 3)
                           for i in range(2)]
        intervals_nonexact = ["[{0}..{1}]".format(v, v + 1)
                              for v in range(exact * 2, exact * 2 + nonexact * 3, 3)]
        column_output.append(output_exact + output_nonexact)
        column_intervals.append(intervals_exact + intervals_nonexact)

    values_list = list(product(*column_values))
    output = list(product(*column_output))
    column_names = ["c%d" % d for d in range(len(column_values))]
    query = make_query(column_names, column_intervals)
    write_test(testname, column_names, values_list, [(query, output)], key_len)
