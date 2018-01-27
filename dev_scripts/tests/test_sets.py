from collections import namedtuple
from itertools import product

column = [x for x in range(2, 10)]
# todo: check every case with key (this should imply rangescan) and without (fullscan)
# it can be checked from .plan file whether rangescan was triggered - as a precondition
interval_pairs = [
    (["[3..5]", "[7..9]"], [3, 4, 5, 7, 8, 9], "no intersection"),
    (["[3..5]", "[4..6]"], [3, 4, 5, 6], "intersection (length 2)"),
    (["[3..4]", "[4..6]"], [3, 4, 5, 6], "intersection (length 1)"),
    (["[3..7)", "(4..6]"], [3, 4, 5, 6], "one interval in other"),
    (["[3..5]", "(5..7]"], [3, 4, 5, 6, 7], "common edge, right edge closed and left open"),
    (["[3..5)", "[5..7]"], [3, 4, 5, 6, 7], "common edge, right edge open and left closed"),
    (["[3..5)", "(5..7]"], [3, 4, 6, 7], "common edge, both sides open")
]

interval_singles = [
    ("[3..)",  [3, 4, 5, 6, 7, 8, 9], "left closed, right unlimited"),
    ("(..7]",  [2, 3, 4, 5, 6, 7], "right closed, left unlimited"),
    ("(3..7]", [4, 5, 6, 7], "left open and right closed"),
    ("[3..7)", [3, 4, 5, 6], "left closed and right open"),
    ("(3..7)", [4, 5, 6], "left and right open"),
    ("[3..7]", [3, 4, 5, 6, 7], "left closed and right closed"),
    ("[3..3]", [3], "left and right closed (length 1)"),
    ("3",      [3], "not range but exact value"),
    ("(..)",   [2, 3, 4, 5, 6, 7, 8, 9], "unlimited"),
    ("[2..2)", [], "empty range - normalized"),
    ("[5..2]", [], "empty range - not normalized")
]
intervals_in_relation_to_data = [
    ("[5..12)", [5, 6, 7, 8, 9], "right boundary exceeded"),
    ("[0..5)", [2, 3, 4], "left boundary exceeded"),
    ("[0..12)", [2, 3, 4, 5, 6, 7, 8, 9], "left and right boundary exceeded"),
    ("[10..12)", [], "right boundary double exceeded"),
    ("[0..2)", [], "left boundary double exceeded")
]
col = namedtuple("Col", ["exact", "nonexact"])
case = namedtuple("Case", ["columns_count", "name", "values", "preds", "keylen", "result"])
plan = namedtuple("Plan", ["fullscan_column"])
plan_tests = [
    case(name="don't rangescan after nonexact filter",
         # cols=[col(exact=0, nonexact=1), col(exact=1, nonexact=0)], keylen=2,
         columns_count=2, keylen=2,
         values=product([0, 1, 2, 3], [0, 1, 2]),
         preds=("[1..2]", "[1..1]"),
         result=plan(fullscan_column=1)),
    case(name="don't rangescan non-key column",
         # cols=[col(exact=1, nonexact=1), col(exact=0, nonexact=0)],
         columns_count=2, keylen=1,
         values=product([0, 1, 2], [0, 1, 2]),
         preds=("[1..1]", "[1..1]"),
         result=plan(fullscan_column=1)),
    case(name="don't rangescan non-key column (first column)",
         # cols=[col(exact=1, nonexact=1), col(exact=0, nonexact=0)],
         columns_count=2, keylen=1,
         values=product([0, 1, 2], [0, 1, 2]),
         preds=("[1..1]", "[1..1]"),
         result=plan(fullscan_column=1)),
    case(name="rangescan column with nonexact filter",
         # cols=[col(exact=1, nonexact=0), col(exact=0, nonexact=1)],
         columns_count=2, keylen=2,
         values=product([0, 1, 2], [0, 1, 2, 3]),
         preds=("[1..1]", "[1..2]"),
         result=plan(fullscan_column=2))
]

class MulticolumnResultTests:
    result = namedtuple("Result", ["rows", "fullscan_request_columns"])
    case = namedtuple("Case", ["columns_count", "name", "values", "preds", "keylen", "result"])
    cases = [
        case(name="fullscan_three_levels",
             columns_count=3, keylen=0,
             values=product([2, 0, 1], [4, 3, 5], [8, 7, 6]),
             preds=(["[0..0]", "[2..2]"], ["[3..3]", "[5..5]"], ["[6..6]", "[8..8]"]),
             result=result(rows=product([2, 0], [3, 5], [8, 6]),
                           fullscan_request_columns=[0, 0, 0])),
        case(name="rangescan_three_levels",
             columns_count=3, keylen=3,
             values=product([0, 1, 2], [3, 4, 5], [6, 7, 8]),
             preds=(["[0..0]", "[2..2]"], ["[3..3]", "[5..5]"], ["[6..6]", "[8..8]"]),
             result=result(rows=product([0, 2], [3, 5], [6, 8]),
                           fullscan_request_columns=[3, 3, 3, 3, 3, 3, 3, 3])),
        case(name="single_rangescan_and_single_fullscan",
             columns_count=2, keylen=2,
             values=product([0, 1], [2, 3, 4]),
             preds=(["[0..0]"], ["[3..4]"]),
             result=result(rows=[(0, 3), (0, 4)],
                           fullscan_request_columns=[1])),
        case(name="single_rangescan_and_single_rangescan",
             columns_count=2, keylen=2,
             values=product([0, 1], [2, 3]),
             preds=(["[0..0]"], ["[3..3]"]),
             result=result(rows=[(0, 3)],
                           fullscan_request_columns=[2])),
        case(name="rangescan_and_fullscan_in_the_same_column",
             columns_count=2, keylen=2,
             values=product([0, 1, 2, 3], [4, 5, 6]),
             preds=(["[0..1]", "[3..3]"], ["[5..6]"]),
             result=result(rows=[(0, 5), (0, 6), (1, 5), (1, 6), (3, 5), (3, 6)],
                           fullscan_request_columns=[1, 2]))
    ]

class OutputOrderingTests:
    case = namedtuple("Case", ["columns_count", "name", "values", "preds", "result"])
    cases = [
        case(name="unordered_single_column",
             columns_count=1,
             values=[7, 3, 5, 1, 2, 6, 4],
             preds=("[2..6]",),
             result=[3, 5, 2, 6, 4]),
        case(name="unordered_two_columns",
             columns_count=1,
             values=product([3, 1, 4, 2], [1, 3, 2, 4]),
             preds=("[2..3]", "[1..3]"),
             result=[(3, 1), (3, 3), (3, 2), (2, 1), (2, 3), (2, 2)])
    ]
