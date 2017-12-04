#pragma once
#include <bits/stdc++.h>
// misc {{{
using u64 = uint64_t;
using u32 = uint32_t;
using i64 = int64_t;
using i32 = int32_t;
const i64 inf = (1ll << 62);
const i64 imax = std::numeric_limits<i64>::max();
const i64 umax = std::numeric_limits<u64>::max();
void merror(std::string msg, i64 line_number) { std::cerr << "assert >> " << line_number << ": " << msg << std::endl; exit(42); }
#ifndef NDEBUG
#define massert(c, msg) do { if (!(c)) merror(msg, __LINE__); } while(0)
#define massert2(c) massert(c, "")
#else
#define massert(...)
#define massert2(...)
#endif
template <typename C> i64 isize(const C& c) { return static_cast<i64>(c.size()); }
template <typename T, size_t N> i64 isize(const std::array<T, N>& = {}) { return static_cast<i64>(N); }
template <typename It, typename Cont> bool it_last(It it, const Cont& c) { return ++it == c.cend(); }
// }}}
// functors {{{
namespace fun {
template <typename T> const auto identity = [](const T& t) -> const T& { return t; };
template <typename T> const auto first = [](const T& t) { return t.first; };
template <typename T> const auto second = [](const T& t) { return t.second; };
template <typename T, u32 n> const auto nth = [](const T& t) { return std::get<n>(t); };
template <typename TOut, typename F, typename TIn> std::vector<TOut> transf(const TIn& in, F f) { std::vector<TOut> v; transform(in.begin(), in.end(), back_inserter(v), f); return v; }
template <typename F, typename T> void sort(T& v, F f) { std::sort(v.begin(), v.end(), f); }
template <typename TOut, typename F, typename TIn> std::vector<TOut> sorted(const TIn& in, F f) { std::vector<TOut> v(in.begin(), in.end()); sort(v, f); return v; }
} // fun
// }}}
// class traits {{{
template<class...> using void_t = void;
template <class, class = void> struct has_repr : std::false_type {};
template <class T> struct has_repr<T, void_t<decltype(std::declval<T>()._repr())>> : std::false_type {};
template <class, class = void> struct has_str : std::false_type {};
template <class T> struct has_str<T, void_t<decltype(std::declval<T>()._str())>> : std::false_type {};
template <class, class = void> struct has_read : std::false_type {};
template <class T> struct has_read<T, void_t<decltype(std::declval<T>()._read())>> : std::false_type {};
template <class, class = void> struct is_map : std::false_type {};
template <class T> struct is_map<T, void_t<typename T::key_type, typename T::mapped_type>> : std::false_type {};
template <class, class = void> struct is_set : std::false_type {};
template <class T> struct is_set<T, void_t<typename T::key_type>> : std::false_type {};
// }}}
// str {{{
template <typename T, class = void> struct _Str { std::string operator()(const T& t) { return to_string(t); } };
template <> struct _Str<std::string> { std::string operator()(const std::string& t) { return t; } };
template <> struct _Str<const char*> { std::string operator()(const char* t) { return t; } };
template <typename T> std::string str(const T& t) { return _Str<T>()(t); }
template <typename T> struct _Str<T, std::enable_if_t<has_str<T>::value>> { std::string operator()(const T& t) {
    return t._str();
} };
template <typename T>
struct _Str<std::vector<T>> {
    std::string operator()(const std::vector<T>& t) {
        std::string s = "";
        for (i64 i = 0; i < isize(t); i++) {
            s += str(t[i]);
            if (i != isize(t) - 1)
                s += ' ';
        }
        return s;
    }
};
// }}}
// string tuples {{{
template <i32 rev_index, typename T> struct _StrTuple { std::string operator() (const T& tpl) {
    return str(std::get<std::tuple_size<T>::value - rev_index>(tpl)) + ' ' + _StrTuple<rev_index - 1, T>()(tpl);
} };
template <typename T> struct _StrTuple<1, T> { std::string operator()(const T& t) {
    return str(std::get<std::tuple_size<T>::value - 1>(t));
} };
template <> struct _StrTuple<0, std::tuple<>> { std::string operator()(const std::tuple<>&) { return ""; } };
template <typename ...Ts> struct _Str<std::tuple<Ts...>> { std::string operator()(const std::tuple<Ts...>& t) {
    using T = std::tuple<Ts...>;
    return _StrTuple<std::tuple_size<T>::value, T>()(t);
} };
template <class T1, class T2> struct _Str<std::pair<T1, T2>> { std::string operator()(const std::pair<T1, T2>& p) {
    return str(make_tuple(p.first, p.second));
} };
// }}}
// repr {{{
template <typename T, class = void> struct _Repr { std::string operator()(const T& t) { return str(t); } };
template <typename T> std::string repr(const T& t) { return _Repr<T>()(t); }
template <> struct _Repr<std::string> { std::string operator()(const std::string& t) { return '"' + t + '"'; } };
template <> struct _Repr<const char*> { std::string operator()(const char* const& t) { return repr(std::string(t)); } };
template <typename T> struct _Repr<T, std::enable_if_t<has_repr<T>::value>> { std::string operator()(const T& t) {
    return t._repr();
} };
template <typename T>
struct _Repr<std::vector<T>> {
    std::string operator()(const std::vector<T>& t) {
        std::string s = "[";
        for (i64 i = 0; i < isize(t); i++) {
            s += repr(t[i]);
            if (i != isize(t) - 1)
                s += ',';
        }
        s += ']';
        return s;
    }
};
template <typename T>
struct _Repr<T, std::enable_if_t<is_set<T>::value && !is_map<T>::value>> {
    std::string operator()(const T& t) {
        std::string s = "{";
        for (auto it = t.cbegin(); it != t.cend(); it++) {
            s += repr(*it);
            if (!it_last(it, t))
                s += ',';
        }
        s += '}';
        return s;
    }
};
template <typename T>
struct _Repr<T, std::enable_if_t<is_map<T>::value>> {
    std::string operator()(const T& t) {
        std::string s = "{";
        for (auto it = t.cbegin(); it != t.cend(); it++) {
            s += repr(it->first) + ':' + repr(it->second);
            if (!it_last(it, t))
                s += ',';
        }
        s += '}';
        return s;
    }
};
// }}}
// struct repr helpers {{{
std::string _repr_helper(i64 i, const std::vector<std::string>& header) {
    massert(i == isize(header), "header too long");
    return "";
}
template <typename T, typename ...Ts>
std::string _repr_helper(i64 i, const std::vector<std::string>& header, const T& t, const Ts&... ts) {
    massert(i < isize(header), "header too short");
    auto s = header[i] + "=" + repr(t);
    if (i < isize(header) - 1)
        s += "," + _repr_helper(i + 1, header, ts...);
    return s;
}
template <typename ...Ts>
std::string make_repr(std::string classname, const std::vector<std::string>& header, const Ts&... ts) {
    return classname + '(' + _repr_helper(0, header, ts...) + ')';
}
/// }}}
// repring tuples {{{
template <i32 rev_index, typename T> struct _ReprTuple { std::string operator() (const T& tpl) {
    return repr(std::get<std::tuple_size<T>::value - rev_index>(tpl)) + ',' + _ReprTuple<rev_index - 1, T>()(tpl);
} };
template <typename T> struct _ReprTuple<1, T> { std::string operator()(const T& t) {
    return repr(std::get<std::tuple_size<T>::value - 1>(t));
} };
template <> struct _ReprTuple<0, std::tuple<>> { std::string operator()(const std::tuple<>&) { return ""; } };
template <typename ...Ts> struct _Repr<std::tuple<Ts...>> { std::string operator()(const std::tuple<Ts...>& t) {
    using T = std::tuple<Ts...>;
    return '(' + _ReprTuple<std::tuple_size<T>::value, T>()(t) + ')';
} };
template <class T1, class T2> struct _Repr<std::pair<T1, T2>> { std::string operator()(const std::pair<T1, T2>& p) {
    return repr(make_tuple(p.first, p.second));
} };
// }}}
// print containers {{{
template <typename It> void _print_collection(std::ostream& os, It begin, It end) { os << "["; while (begin != end) { os << *(begin++); if (begin != end) os << ','; } os << "]"; }
#define MAKE_PRINTER_1(container) template <typename T> std::ostream& operator<<(std::ostream& os, const container<T>& t) { _print_collection(os, t.begin(), t.end()); return os; }
#define MAKE_PRINTER_2(container) template <typename T1, typename T2> std::ostream& operator<<(std::ostream& os, const container<T1, T2>& t) { _print_collection(os, t.begin(), t.end()); return os; }
MAKE_PRINTER_1(std::vector)
MAKE_PRINTER_2(std::map)
MAKE_PRINTER_1(std::set)
MAKE_PRINTER_2(std::unordered_map)
MAKE_PRINTER_1(std::unordered_set)
#undef MAKE_PRINTER_1
#undef MAKE_PRINTER_2
// }}}
// read/write {{{
template <typename T, std::enable_if_t<!has_read<T>::value, int> = 0> T read() { T e; std::cin >> e; return e; }
template <typename T, std::enable_if_t<has_read<T>::value, int> = 0> T read() { T t; t._read(); return t; }
void read() {}
template <typename T, typename ...Ts> void read(T& v, Ts& ...ts) { v = read<T>(); read(ts...); }
template <typename T> std::vector<T> readv(i64 n) { std::vector<T> v; readv(n, v); return v; }
template <typename T> void readv(i64 n, std::vector<T>& v) { for (i64 i = 0; i < n; i++) v.push_back(read<T>()); }
#define PRINTERS(FNAME, OUTP) \
    void FNAME() {} \
    template <typename T> void FNAME(const T& t) { OUTP << t; } \
    template <typename T1, typename T2, typename ...Ts> \
    void FNAME(const T1& t1, const T2& t2, const Ts& ...ts) { OUTP << t1 << ' '; FNAME(t2, ts...); } \
    template <typename ...Ts> \
    void FNAME##ln(const Ts& ...ts) { FNAME(ts...); FNAME('\n'); } \
    template <typename T, typename F> void FNAME##v(const T& t, F f) { for (i64 i = 0; i < isize(t); i++) if (i == isize(t) - 1) FNAME(f(t[i])); else FNAME(f(t[i]), ""); FNAME##ln(); } \
    template <typename T> void FNAME##v(const T& t) { FNAME##v(t, fun::identity<typename T::value_type>); }
PRINTERS(print, std::cout)
PRINTERS(lprint, std::cerr)
#ifdef DEBUG_PRINTS
    PRINTERS(dprint, std::cerr)
#else
# define dprint(...)
# define dprintv(...)
# define dprintln(...)
#endif
/// }}}
