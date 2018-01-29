# planty-db

Tool for querying data in better complexity than `O(n)`.

`plantydb` reads CSV-like file along with information about sort order, and then answers SQL-like queries, trying to avoid scanning everything. It uses binary search whenever possible.

In practice, it can be used in combination with other database, e.g. SQLite, as a faster alternative for specific queries.

During program run, data is stored 100% in RAM and is read-only.

## What kind of queries?

- Selecting columns in CSV-like file (with integer numbers)
- Filtering column values with `<`, `=` or `>` predicates (e.g. `year > 1980`)
- Most logical relations between predicates can be expressed, e.g. `income < 500 and (year < 1980 or year > 2000)`.

## Language

    select <comma-separated column_list (or *)> [where <comma-separated condition list>]
    # e.g.
    select a, b where a=0, b=2
    select a
    select * where b=[0..)

Expressions have two forms - `column=value` or `column=<interval>`:

    select a where a=0
    select a where a=[0..2)
    select a where a=(2..4]
    select a where a=(..2]

Interval can be open, closed or unlimited on either side. Open interval starts/ends with `(`/`)`. Closed starts/ends with `[`/`]`. Unlimited interval don't have a value, e.g. `(..5]`, `(2..)` or `(..)`.

### How are logical AND/OR expressed?

At first, predicates are grouped by column, and joined with OR expression. Then all groups are AND-ed together.

Let's see some examples. Let's start with single column.

    SQL:
        SELECT a FROM t WHERE a=2 OR a=4 OR a>10;
    plantydb:
        SELECT a WHERE a=2, a=4, a=(10..)
 
Explanation: expressions for the same column are treated as in they were in OR expression.
For single column, it allows `plantydb` to express everything `SQL` can.

Negation example:

    SQL:
        SELECT a FROM t WHERE NOT a=2;
    plantydb:
        SELECT a WHERE a=(..2), a=(2..)

Let's move on to expressions involving many columns.

    SQL:
        SELECT * FROM t WHERE ((year>1980 AND year<=2000) OR year=2018) AND debt>0
    plantydb:
        SELECT * WHERE year=(1980..2000), year=2018, debt>0
    other version:
        SELECT * WHERE year=(1980..2000), debt>0, year=2018

It works, because expressions for different column are treated as alternative.

### Why not SQL? (author's motivation)

For a small, backend-focused project like this, I wanted parsing logic to be as simple as possible.

Giving it better look
(e.g. adding AND/OR keywords in place of commas) is on a TODO list. Full expression trees will most likely
be never implemented.

# Example usage
Database is run with one argument - path to a database file. Queries are read from `stdin`. Query result (output csv or error) is written to `stdout`. Debug info, performance info, serious errors, all go to `stderr`. 

    > plantydb database.csv
    >> select * where year=[1980..2000), income[500..1000)
    year income debt
    1980 600 123
    1993 400 151

# Benchmarks

test name | planty-db | sqlite3 :memory: | sqlite3 from file | test description
--- | --- | --- | --- | ---
test1 | 0.12s | 0.39s | 0.41s | single column, range scan, single interval
test2 | 0.04s | 0.43s | 0.36s | single column, full scan, single interval
test3 | 0.67s | 2.2s | 2.2s | single column, range scan, query for 100 ranges
test4 | 2s | 5.8s | 5.7s | single column, full scan, query for 100 ranges
test5 | 1.58s | 60s | 60s | query for exact row in 20-column table
test6 | 1.07s | 8.42s | 4.11s | 13 columns, range scan, conditions are like `(c0=0 or c0=2) and (c1=0 or c1=2) and ...`
test7 | 4.33s | 3.84s | 3.56s | full scan for ~10 columns
test8 | 2.4s | 7.8s | 6.3s | full scan 3 columns, 1'000'000 values
test9 | 0.61s | 2.2s | 1.94s | full scan 2 columns

SQLite table was created with primary key and WITHOUT ROWID.
Test data can be reproduced using [generate_perf.py](planty-db/dev_scripts/generate_perf.py).
Preset config is [here](planty-db/dev_scripts/perf_presets.yaml).

Those benchmarks are just for perspective. They should not be taken too seriously, because test cases were not very considerate, and methodology was a bit sloppy.

In case of "sqlite from file" test, database was prepared before measurement. In both other cases, loading data from file - in case of SQLite, INSERT statements - is also part of measurement.

SQLite version is 3.11.0. Test measurement was made using `time` linux utility, on typical modern computer - Ubuntu Linux 16.04, with Intel i7 4700hq processor, SSD storage, and more than enough RAM.


# Database file format
    col1 col2; 1
    0 1312
    0 112
    0 210
    20 11

Number after the semicolon is a number of columns by which the data is initially sorted. It can be any number, from zero to total number of columns.

The data ordering is used to perform queries without full scan.

# Build instructions
    > cd src/
    > make release.e

For more build targets and compilation options, see `src/Makefile`.

# Technical info
- c++17
- aside from that, no library dependencies

# Running tests
    # required: python3.5, pytest
    dev_scripts/run_tests.py
