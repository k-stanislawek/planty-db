# planty-db
Relational database for storing event-like data, operating in RAM
- powered from a csv file
- primary key on multiple columns supported
- sql-like language:
    select year, income where year=(1982..1994], income=[20..30)
    -- SQL equivalent: select year, income from t where year > 1982 and year <= 1994 and income >= 20 and income < 30
    select *
    select * where a<1000, a>2000, b=300
    -- SQL equivalent: select * from t where a < 1000 or a > 2000 and b = 300
every SQL WHERE clause with expressions limited to `column </=/> constant` can be represented and executed in this database.

# example:
    > plantydb database.csv
    >> select * where col1=[0..1), col3=[1..)
    col1 col2 col3 col4
    0 0 1 0
    0 0 1 1
    0 1 1 0
    0 1 1 1

## technical:
- c++17
