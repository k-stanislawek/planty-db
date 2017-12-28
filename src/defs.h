#pragma once
#include <bits/stdc++.h>
#include "basic.h"

using index_t = i64;
using value_t = i64;
using vstr = std::vector<std::string>;
using indices_t = std::vector<index_t>;
using vi64 = std::vector<i64>;
using cname = std::string;
using cnames = std::vector<std::string>;
using std::vector;
using std::string;
template <typename T> auto to_str(T&& t) { return std::to_string(t); }

#define log_info(...) lprintln("info:", __VA_ARGS__)
#define log_debug(...) dprintln("debug:", __VA_ARGS__)
#define log_perf(...) lprintln("perf:", __VA_ARGS__)

#define bound_assert(INDEX, CONTAINER) massert(INDEX >= 0 && INDEX < isize(CONTAINER), \
        "Index '" #INDEX "' = " + std::to_string(INDEX) + " out of bounds of container '" #CONTAINER \
        "' with size " + std::to_string(isize(CONTAINER)))
#define unreachable_assert(msg) do { std::cerr << "Fatal error: unreachable code was reached in line " << __LINE__ << ", message: " << msg << std::endl; exit(42); } while(0)


