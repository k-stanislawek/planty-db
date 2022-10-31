# planty-db

A small, "database"-like prototype program to query a sorted-CSV-like data efficiently (leveraging binary search). It's been implemented to see if a custom c++ "database", that is specialized at a specific use-case, but still generic over data, can compete, or even beat, a renowned database, like SQLite.

It turns out that yeah, it can (with many caveats)! (My own measurements are at the end of this file.)
However, this is not a practical tool at all.

## Features / limitations

### Input dataset format

**planty-db** is launched like this: `plantydb file.csv`, and on startup, loads the whole file into RAM. The `file.csv` looks like this:

```
col1 col2 col3; 2
1 20 900
4 10 800
5 10 700
5 20 600
5 40 500
6 10 400
10 10 300
```
First line is a header, as well as one extra number - number of leading columns that the file is sorted by (like a "primary key").

The database supports exactly one such "table", it's fully loaded on startup and immutable. Basically, updating is out of scope of this prototype.

### Data types

Only numbers supported - 64-bit signed integers.

### Queries / mistakes

A sql-like, quick-and-dirty query format has been implemented. Example queries:

    select *
    # In SQL, this would be SELECT * FROM sth, but "from" clause is skipped.

    select col1, col2
    # You can specify columns as well.

    select * where col1=0, col2=2
    # Filter by values. Predicates of different columns are all enforced in the output.

    select * where col1=0, col1=1
    # This time, both predicates address the same column. Multiple predicates on the same column
    # have highest preference, and are considered alternatives.
    # So, both rows with col1=0 and col1=1 will be returned.
    # I realize it's confusing, I think it's been a mistake.
    # Putting it all together:
    select * where col1=0, col1=1, col2=1
    # This is equivalent to SQL:
    # SELECT * FROM <somewhere> WHERE (col1=0 OR col1=1) AND col2=1
    # It's been done like this to support the most common-sense use-cases, without supporting nested expressions.

    # Regarding predicates - aside from "=NUM" seen above, there's also "=RANGE".
    # It can be used anywhere in place of "=NUM":
    select * where col1=[0..5]
    # Intervals can also be open-ended on either side:
    select * where col1=(2..10], col2=[3..8)
    # ... as well as unlimited:
    select * where col1=(2..), col2=(..10)


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

In case of "sqlite from file" test, database was prepared before measurement. In both other cases, loading data from file - in case of SQLite, INSERT statements - is also part of measurement.

SQLite version is 3.11.0. Test measurement was made using `time` linux utility, on typical modern computer - Ubuntu Linux 16.04, with Intel i7 4700hq processor, SSD storage, and more than enough RAM.


# Build instructions
    > cd src/
    > make
    > make release.e

For more build targets and compilation options, see `src/Makefile`.

# Technical info
- c++17
- aside from that, no library dependencies

# Running tests
    # required: python3.5+, pytest
    python dev_scripts/run_tests.py

# Benchmark
    # required: python3.5+, pytest, ruamel.yaml

