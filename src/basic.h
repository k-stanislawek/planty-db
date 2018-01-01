#pragma once
#include <bits/stdc++.h>

// misc {{{
using u64 = uint64_t;
using u32 = uint32_t;
using i64 = int64_t;
using i32 = int32_t;
void merror(std::string msg, const char* file, i64 line_number)
    { std::cerr << file << ':' << line_number << ": " << msg << std::endl; exit(42); }
#ifndef NDEBUG
#define massert(c, msg) do { if (!(c)) merror(msg, __FILE__, __LINE__); } while(0)
#define massert2(c) massert(c, "")
#else
#define massert(...) do {} while(0);
#define massert2(...) do {} while(0);
#endif
// }}}
// functors {{{
template <class C> i64 isize(C const& c) { return static_cast<i64>(c.size()); }
template <class T, size_t N> i64 isize(std::array<T, N> const&) { return static_cast<i64>(N); }
namespace fun {
/* on ranges */
auto map = [](auto const& c, auto const& f) {
    std::vector<typename std::decay<decltype(f(*c.begin()))>::type> v;
    for (auto const& e : c) v.push_back(f(e));
    return v;
};
auto sort = [](auto & c, auto &&...ts) { std::sort(c.begin(), c.end(), ts...); };
auto sorted = [](auto const& c, auto &&...ts) { auto c2 = c; sort(c2, std::forward(ts)...); return c2; };
auto lex_compare = [](auto const& r1, auto const& r2)
    { return std::lexicographical_compare(r1.begin(), r1.end(), r2.begin(), r2.end()); };
/* extractors */
auto first = [](auto const& e) { return e.first; };
auto second = [](auto const& e) { return e.second; };
template <int n> auto nth = [](auto const& e) { return std::get<n>(e); };
/* string related */
auto surround = [](auto const& a, char b, char c = '\0') { return b + a + (c ? c : b); };
auto join = [](auto const& container, auto const& separator, auto const& mapper) {
    std::string s;
    if (container.empty()) return s;
    auto it = container.begin(); s += mapper(*it++);
    for (; it != container.end(); it++) { s += separator; s += mapper(*it++); }
    return s;
};
} // namespace fun
template <class T> auto vector_less(T const& a, T const& b) { return fun::lex_compare(a, b); }
/* tuple */
template<class F, class T, class Seq = std::make_integer_sequence<int, std::tuple_size<T>::value>>
struct TupleForeach;
template<class F, class T, int ...nums>
struct TupleForeach<F, T, std::integer_sequence<int, nums...>> { auto operator()(F const& f, T const& t) {
    // variadic parameter pack expansion's usage is ugly: https://stackoverflow.com/a/26912970
    [[maybe_unused]] int unused[] = {0, (f(nums, std::get<nums>(t)), 0)...};
}};
template <class F, class T> void tuple_foreach(F const& f, T const& t)
    { TupleForeach<F, T>()(f, t); }
template <class F, class S, class ...Ts>
std::string join_tuple(std::tuple<Ts...> const& tpl, S const& separator, F const& mapper) {
    std::string res;
    tuple_foreach([&](i64 num, auto const& e) {
        if (num != 0) res += separator;
        res += mapper(e);
    }, tpl);
    return res;
}
// }}}
// class traits {{{
template<class...> using void_t = void;
template <class, class = void> struct has_repr : std::false_type {};
template <class T> struct has_repr<T, void_t<decltype(std::declval<T>()._repr())>> : std::true_type {};
template <class, class = void> struct has_str : std::false_type {};
template <class T> struct has_str<T, void_t<decltype(std::declval<T>()._str())>> : std::true_type {};
template <class, class = void> struct has_read : std::false_type {};
template <class T> struct has_read<T, void_t<decltype(std::declval<T>()._read())>> : std::true_type {};
template <class, class = void> struct is_map : std::false_type {};
template <class T> struct is_map<T, void_t<typename T::key_type, typename T::mapped_type>> : std::true_type {};
template <class, class = void> struct is_set : std::false_type {};
template <class T> struct is_set<T, void_t<typename T::key_type>> : std::true_type {};
template <class T> constexpr bool _is_char_ptr()
    { return std::is_same<typename std::decay<T>::type, char*>::value; }
template <class, class = void> struct is_ptr : std::false_type {};
template <class ...Ts> struct is_ptr<std::shared_ptr<Ts...>> : std::true_type {};
template <class ...Ts> struct is_ptr<std::unique_ptr<Ts...>> : std::true_type {};
template <class ...Ts> struct is_ptr<std::weak_ptr<Ts...>> : std::true_type {};
template <class T> struct is_ptr<T, void_t<std::enable_if_t<std::is_pointer_v<T> && !_is_char_ptr<T>()>>>
    : std::true_type {};
// }}}
// str {{{
template <class T, class = void> struct _Str { auto operator()(T const& t)
    { return std::to_string(t); }};
template <class T> auto str(const T& t)
    { return _Str<T>()(t); }
template <> struct _Str<std::string> { auto operator()(const std::string& t)
    { return t; }};
template <> struct _Str<std::string_view> { auto operator()(const std::string_view& t)
    { return std::string(t); }};
template <class T> struct _Str<T, std::enable_if_t<_is_char_ptr<T>()>> { auto operator()(T const& t)
    { return std::string(t); }};
template <class T> struct _Str<T, std::enable_if_t<is_ptr<T>::value>> { auto operator()(T const& t)
    { return t ? str(*t) : str("(nullptr)"); }};
template <class T> struct _Str<T, std::enable_if_t<has_str<T>::value>> { auto operator()(T const& t)
    { return t._str(); }};
template <class T> struct _Str<std::reference_wrapper<T>> { auto operator()(T const& t)
    { return str(t.get()); }};
template <class T> struct _Str<std::optional<T>> { auto operator()(T const& t)
    { return t ? str(*t) : str(""); }};
struct StrFunctor { template <class T> auto operator()(T const& t) const; };
template <class T> struct _Str<std::vector<T>> { auto operator()(std::vector<T> const& t)
    { return fun::join(t, ' ', StrFunctor()); }};
template <class ...Ts> struct _Str<std::tuple<Ts...>> { auto operator()(std::tuple<Ts...> const& t)
    { return join_tuple(t, ' ', StrFunctor()); }};
template <class T1, class T2> struct _Str<std::pair<T1, T2>> { auto operator()(const std::pair<T1, T2>& p)
    { return str(make_tuple(p.first, p.second)); }};
template <class T> auto StrFunctor::operator()(T const& t) const
    { return str(t); }
// }}}
// repr {{{
template <class T, class = void> struct _Repr { auto operator()(T const& t)
    { return str(t); }};
template <class T> auto repr(const T& t)
    { return _Repr<T>()(t); }
template <> struct _Repr<std::string> { auto operator()(const std::string& t)
    { return fun::surround(str(t), '"'); }};
template <> struct _Repr<std::string_view> { auto operator()(const std::string_view& t)
    { return fun::surround(str(t), '"'); }};
template <class T> struct _Repr<T, std::enable_if_t<_is_char_ptr<T>()>> { auto operator()(T const& t)
    { return fun::surround(str(t), '"'); }};
template <class T> struct _Repr<T, std::enable_if_t<is_ptr<T>::value>> { auto operator()(T const& t)
    { return t ? repr(*t) : str(t); }};
template <class T> struct _Repr<T, std::enable_if_t<has_repr<T>::value>> { auto operator()(T const& t)
    { return t._repr(); }};
template <class T> struct _Repr<T,
         std::enable_if_t<!has_repr<T>::value && has_str<T>::value>> { auto operator()(T const& t)
    { return str(t); }};
template <class T> struct _Repr<std::reference_wrapper<T>> { auto operator()(T const& t)
    { return repr(t.get()); }};
template <class T> struct _Repr<std::optional<T>> { auto operator()(T const& t)
    { return t ? fun::surround(repr(*t), '<', '>') : str("<None>"); }};
struct ReprFunctor { template <class T> auto operator()(T const& t) const; };
template <class T> struct _Repr<std::vector<T>> { auto operator()(std::vector<T> const& t)
    { return fun::surround(fun::join(t, ',', ReprFunctor()), '[', ']'); }};
template <class T> struct _Repr<T, std::enable_if_t<is_set<T>::value && !is_map<T>::value>>
        { auto operator()(T const& t)
    { return fun::surround(fun::join(t, ',', ReprFunctor()), '{', '}'); }};
auto _repr_map_item = [](auto const& item) { return join_tuple(item, ':', ReprFunctor()); };
template <class T> struct _Repr<T, std::enable_if_t<is_map<T>::value>> { auto operator()(T const& t)
    { return fun::surround(fun::join(t, ',', _repr_map_item), '{', '}'); }};
template <class T> auto ReprFunctor::operator()(T const& t) const
    { return repr(t); }
template <class ...Ts>
std::string make_repr(std::string classname, const std::vector<std::string>& header, const Ts&... ts) {
    massert(isize(header) == sizeof...(ts), "header wrong length");
    std::string res;
    tuple_foreach([&](auto i, auto const& v) {
        if (i != 0) res += ',';
        res += header[i] + "=" + repr(v);
    }, std::forward_as_tuple(ts...));
    return classname + fun::surround(res, '(', ')');
}
/// }}}
// read {{{
template <class T> T read()
    { T e; std::cin >> e; return e; }
template <class T, class ...Ts> void read(T& v, Ts& ...ts)
    { v = read<T>(); if constexpr(sizeof...(ts)) read(ts...); }
template <class T> void readv(i64 n, std::vector<T>& v)
    { std::generate_n(std::back_inserter(v), n, read<T>); }
template <class T> std::vector<T> readv(i64 n)
    { std::vector<T> v; readv(n, v); return v; }
// }}}
// write {{{
template <char c> struct Printer {
    template <class ...Ts> void operator()(Ts const&... ts)
        { outp << str(std::forward_as_tuple(ts...)) << c; }
    std::ostream& outp;
};
auto print = Printer<' '>{std::cout};
auto println = Printer<'\n'>{std::cout};
auto lprint = Printer<' '>{std::cerr};
auto lprintln = Printer<'\n'>{std::cerr};
#ifdef DEBUG_PRINTS
auto dprint = Printer<' '>{std::cerr};
auto dprintln = Printer<'\n'>{std::cerr};
#else
template <class ...Ts> void dprint(Ts const&...) {}
template <class ...Ts> void dprintln(Ts const&...) {}
#endif
// }}}


