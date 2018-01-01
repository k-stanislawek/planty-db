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

auto log_info = [](auto const& ...ts) { lprintln("info:", ts...); };
auto log_plan = [](auto const& ...ts) { lprintln("plan:", ts...); };
auto log_debug = [](auto const& ...ts) { lprintln("debug:", ts...); };
#ifdef PERF_PRINTS
auto log_perf = [](auto const& ...ts) { lprintln("perf:", ts...); };
#else
auto log_perf = [](auto const& ...) { };
#endif

#define bound_assert(INDEX, CONTAINER) massert(INDEX >= 0 && INDEX < isize(CONTAINER), \
        "Index '" #INDEX "' = " + std::to_string(INDEX) + " out of bounds of container '" #CONTAINER \
        "' with size " + std::to_string(isize(CONTAINER)))
#define unreachable_assert(msg) do { std::cerr << "Fatal error: unreachable code was reached in line " << __LINE__ << ", message: " << msg << std::endl; exit(42); } while(0)

