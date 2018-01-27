#include <bits/stdc++.h>
#include "basic.h"
#include "metaclass.h"
#include "defs.h"
#include "measure.h"
#include "ranges.h"

using namespace std::string_literals;

// {{{ exceptions and checks definitions
#define exception_check(ERROR_NAME, COND, ...) \
    do { if (!(COND)) throw ERROR_NAME(__VA_ARGS__); } while(0)

#define add_exception(ERROR_NAME, PARENT) struct ERROR_NAME : public PARENT \
    { template <typename ...Ts> ERROR_NAME(Ts const& ...msg) : PARENT(str(std::forward_as_tuple(msg...))) {} }

add_exception(data_error, std::runtime_error);

add_exception(query_format_error, data_error);
#define query_format_check(COND, ...) exception_check(query_format_error, COND, __VA_ARGS__)

add_exception(table_error, data_error);
#define table_check(COND, ...) exception_check(table_error, COND, __VA_ARGS__)

add_exception(query_semantics_error, data_error);
#define query_semantics_check(COND, ...) exception_check(query_semantics_error, COND, __VA_ARGS__)

add_exception(lexical_cast_error, data_error);
#define lexical_cast_check(COND, ...) exception_check(lexical_cast_error, COND, __VA_ARGS__)

#undef add_exception
// }}}
class RowRange { // {{{
public:
    using iterator = IntIterator<index_t>;
    RowRange() = default;
    RowRange(RowRange const&) = default;
    RowRange & operator=(RowRange const&) = default;
    RowRange & operator=(RowRange &&) = default;
    RowRange(RowRange &&) = default;
    RowRange(index_t l0, index_t r0) : l_(l0), r_(r0) { normalize(); }
    static RowRange make_empty() noexcept { return RowRange(1, 0); }
    bool operator==(const RowRange& other) const { return l_ == other.l_ && r_ == other.r_; }
    bool operator<(const RowRange& other) const { return l_ != other.l_ ? l_ < other.l_ : r_ < other.r_; }
    index_t l() const { return l_; }
    index_t& l() { return l_; }
    index_t r() const { return r_; }
    index_t& r() { return r_; }
    index_t len() const { return r_ - l_ + 1; }
    bool empty() const { return l_ > r_; }
    void normalize() { l_ = std::min(l_, r_ + 1); }
    auto begin() const { return iterator(l_); }
    auto end() const { return iterator(r_ + 1); }
    string _str() const { return empty() ? string("<>") : ("<" + str(l_) + ".." + str(r_) + ">"); }
    string _repr() const { return make_repr("RowRange", {"l", "r"}, l_, r_); }
private:
    index_t l_, r_;
}; // }}}
// ValueInterval {{{
constexpr char const* interval_separator = "..";
class ValueInterval {
    static i64 parse_i64_(string_view strv) {
        auto [res, is_ok] = to_i64(strv);
        query_format_check(is_ok, "Error during converting to integer:", strv);
        return res;
    }
public:
    ValueInterval(string_view strv) {
        auto const sep_pos = strv.find(interval_separator);
        l_open_ = r_open_ = l_infinity_ = r_infinity_ = false;
        l_ = r_ = 0;
        if (sep_pos == string_view::npos) {
            // exact value
            auto const val = parse_i64_(strv);
            l_ = r_ = val;
            return;
        }
        // e.g. [1..2), (1..4], (..3), (..), [2..)
        // todo: regexp
        query_format_check(sep_pos > 0 && sep_pos + 2 < strv.size(), "bad '..' position: ", strv);
        l_open_ = strv.front() == '(';
        r_open_ = strv.back() == ')';
        query_format_check(l_open_ || strv.front() == '[', "bad range open: ", strv);
        query_format_check(r_open_ || strv.back() == ']', "bad range close: ", strv);
        l_infinity_ = (sep_pos == 1);
        r_infinity_ = (sep_pos + 2 == strv.size() - 1);
        if (!l_infinity_)
            l_ = parse_i64_(strv.substr(1, sep_pos - 1));
        if (!r_infinity_)
            r_ = parse_i64_(strv.substr(sep_pos + 2, strv.size() - 1 - (sep_pos + 2)));
    }
    ValueInterval(value_t l0, value_t r0, bool l_open0 = false, bool r_open0 = false,
        bool l_infinity0 = false, bool r_infinity0 = false) noexcept : l_(l0), r_(r0), l_open_(l_open0),
            r_open_(r_open0), l_infinity_(l_infinity0), r_infinity_(r_infinity0) {
        normalize_();
    }
    void normalize_() noexcept {
        if (l_infinity() || r_infinity())
            return;
        if (l() < r())
            return;
        if (l() == r() && !l_open() && !r_open())
            return;
        r_ = l() - 1;
        r_open_ = true;
        l_open_ = false;
    }
    bool empty() const noexcept { return !l_infinity() && !r_infinity() && r() < l(); }
    value_t const& l() const noexcept { return l_; }
    value_t const& r() const noexcept { return r_; }
    bool l_open() const noexcept { return l_open_; }
    bool r_open() const noexcept { return r_open_; }
    bool l_infinity() const noexcept { return l_infinity_; }
    bool r_infinity() const noexcept { return r_infinity_; }
    bool is_single_value() const noexcept {
        return !l_infinity_ && !r_infinity_ && !l_open_ && !r_open_ && l_ == r_;
    }
    bool contains(value_t const& v) const noexcept {
        bool const l_contains = l_infinity_ || (l_open_ ? (l_ < v) : (l_ <= v));
        bool const r_contains = r_infinity_ || (r_open_ ? (r_ > v) : (r_ >= v));
        return l_contains && r_contains;
    }
    string _repr() const {
        auto bracket1 = (l_open() ? '(' : '[');
        auto bracket2 = (r_open() ? ')' : ']');
        auto value1 = (l_infinity() ? ""s : repr(l_));
        auto value2 = (r_infinity() ? ""s : repr(r_));
        return bracket1 + value1 + ".." + value2 + bracket2;
    }
    string _str() const { return _repr(); }

    auto get_key() const
        { return std::make_tuple(l_helper_(), l_open_, r_helper_(), !r_open_); }
    friend bool operator<(ValueInterval const& a, ValueInterval const& b)
        { return a.get_key() < b.get_key(); }

    bool neighbors_right(const ValueInterval& other) const {
        massert2(get_key() <= other.get_key());
        if (r_helper_() != other.l_helper_())
            return r_helper_() > other.l_helper_();
        return !r_open_ || !other.l_open_;
    }
    void merge_right(const ValueInterval& other) {
        massert2(neighbors_right(other));
        if (r_helper_() > other.r_helper_())
            return;
        if (r_helper_() == other.r_helper_()) {
            r_open_ = std::min(r_open_, other.r_open_);
        } else {
            r_open_ = other.r_open_;
            r_ = other.r_;
            r_infinity_ = other.r_infinity_;
        }
    }

private:
    value_t l_helper_() const {
        if (l_infinity_)
            return std::numeric_limits<value_t>::min();
        return l_;
    }
    value_t r_helper_() const {
        if (r_infinity_)
            return std::numeric_limits<value_t>::max();
        return r_;
    }
    value_t l_, r_;
    bool l_open_, r_open_, l_infinity_, r_infinity_;
}; // }}}
class IntColumn { // {{{
public:
    using ref = std::reference_wrapper<const IntColumn>;
    using ptr = std::unique_ptr<IntColumn>;

    static ptr make() { return std::make_unique<IntColumn>(); }

    void push_back(const value_t elem) { data_.push_back(elem); }
    cname& name() noexcept { return name_; }
    const cname& name() const noexcept { return name_; }
    const value_t& at(index_t index) const noexcept { bound_assert(index, data_); return data_[index]; }
    index_t rows_count() const noexcept { return isize(data_); }
    RowRange equal_range(const RowRange& rng, value_t val) const noexcept {
        auto const r = std::equal_range(data_.begin() + rng.l(), data_.begin() + rng.r() + 1, val);
        return RowRange(r.first - data_.begin(), r.second - data_.begin() - 1);
    }
    RowRange equal_range(RowRange const& rng, ValueInterval const& val) const noexcept {
        // todo:
        // class IntervalSide: // two values: left and right
        //     Side other() const;
        //     bool operator==(Side other) const;
        //     ...
        // class ValueIntervalSide {
        //     ValueIntervalSide(Side side, string_view value, string_view paren);
        //     Side side() const;
        //     bool left() const;
        // private:
        //    side_, value_, infinity_, open_;
        // }
        // class RowIntervalSide:
        //     RowIntervalSide(Side side, i64 index);
        //     narrow();
        //     i64 first_index_behind()
        //         return index_ + (side_.left() ? -1 : 1);
        if (val.empty())
            return RowRange::make_empty();
        auto l = rng.l();
        if (!val.l_infinity()) {
            auto const l_rng = equal_range(rng, val.l());
            l = val.l_open() ? (l_rng.r() + 1) : l_rng.l();
        }
        auto r = rng.r();
        if (!val.r_infinity()) {
            auto const r_rng = equal_range(rng, val.r());
            r = val.r_open() ? (r_rng.l() - 1) : r_rng.r();
        }
        return RowRange(l, r);
    }
    string _repr() const { return make_repr("IntColumn", {"name", "length"}, name_, isize(data_)); }
private:
    vector<value_t> data_;
    cname name_;
}; // }}}
class RowNumbers; // {{{
class RowNumbersEraser {
public:
    RowNumbersEraser(RowNumbers & rows) : rows_(rows) { indices_.reserve(65536); }
    ~RowNumbersEraser() noexcept;
    // note: opportunity for optimization
    void keep(index_t idx) { massert2(indices_.empty() || indices_.back() < idx); indices_.push_back(idx); }
private:
    RowNumbers & rows_;
    indices_t indices_;
};
class RowNumbers {
    friend class RowNumbersEraser;
public:
    RowNumbers(RowRange const& range) noexcept : range_(range) {}
    RowRange as_range() const noexcept {
        massert2(!indices_set_used_);
        return range_;
    }
    void narrow(RowRange narrower) noexcept {
        massert(!indices_set_used_, "indices_set_ used during narrowing");
        range_.l() = std::max(range_.l(), narrower.l());
        range_.r() = std::min(range_.r(), narrower.r());
        range_.normalize();
    }
    template <typename F>
    void foreach(const F& f) const {
        if (indices_set_used_)
            for (const auto& row_id : indices_)
                f(row_id);
        else
            for (auto const row_id : range_)
                f(row_id);
    }
    template <typename F>
    static void foreach(vector<RowNumbers> const& rows, F const& f) {
        for (auto const& r : rows)
            r.foreach(f);
    }
    string _str() const {
        return indices_set_used_ ? "{" + str(isize(indices_)) + " rows}" : str(range_);
    }
private:
    RowRange range_;
    indices_t indices_ = {};
    bool indices_set_used_ = false;
};
RowNumbersEraser::~RowNumbersEraser() noexcept {
    massert2(!rows_.indices_set_used_);
    if (isize(indices_) == rows_.range_.len())
        return;
    rows_.indices_set_used_ = true;
    rows_.indices_ = move(indices_);
}
// }}}
// metadata {{{
class Metadata {
public:
    Metadata(vstr&& columns0, i64 key_len0) : columns_(columns0), key_len_(key_len0) {
        massert2(!columns_.empty());
        table_check(key_len_ <= columns_count(),
                "key_len", key_len_, "exceeds header length", columns_count());
    }
    i64 columns_count() const { return isize(columns_); }
    index_t column_id(const cname& name) const { return resolve_column_(name); }
    indices_t column_ids(const cnames& names) const {
        indices_t res;
        for (auto const& name : names)
            res.push_back(resolve_column_(name));
        return res;
    }
    auto const& column_name(i64 const& id) const { bound_assert(id, columns_); return columns_[id]; }
    auto const& column_names() const { return columns_; }
    IntRange columns() const { return IntRange(0, columns_count()); }
    auto const& key_len() const { return key_len_; }
    IntRange key_columns() const { return IntRange(0, key_len()); }
    string _repr() const { return make_repr("Metadata", {"columns", "key_len"}, columns_, key_len_); }
private:
    index_t resolve_column_(cname const& name) const {
        for (auto const i : columns())
            if (columns_[i] == name)
                return i;
        throw table_error("unknown column name: " + name);
    }
    vstr columns_;
    i64 key_len_;
};
// }}}
// input, output frame {{{
class InputFrame {
public:
    InputFrame(std::istream& is) : is_(is) {
    }
    // todo: rewrite whole read/write part
    Metadata get_metadata() {
        char sep = '\0';
        vstr header;
        i64 key_len = 0;
        bool metadata_found = false;
        while (sep != '\n' && !metadata_found) {
            string s;
            is_ >> s;
            dprintln("s", s);
            table_check(!is_.eof(), "empty header");
            if (s.back() == ';') {
                s.pop_back();
                metadata_found = true;
            }
            header.push_back(s);
            is_.read(&sep, 1);
            table_check(!(sep != '\n' && sep != '\t' && sep != ' ' && sep != ';'),
                    "Bad separator: ascii code", static_cast<int>(sep));
        }
        if (metadata_found)
            is_ >> key_len;
        ++(*this);
        dprintln("header", header);
        auto m = Metadata(move(header), move(key_len));
        dprintln(repr(m));
        return m;
    }
    InputFrame& operator++() { is_ >> val_; return *this; }
    bool end() const { return is_.eof(); }
    value_t operator*() { return val_; }
private:
    value_t val_;
    std::istream& is_;
};
class OutputFrame {
public:
    OutputFrame(std::ostream& os) : os_(os) {
    }
    void add_header(const vstr& header) {
        table_check(!header.empty(), "header can't be empty");
        os_ << header[0];
        for (i64 i = 1; i < isize(header); i++)
            os_ << ' ' << header[i];
    }
    void new_row(const i64& val) { os_ << '\n' << val; }
    void add_to_row(const i64& val) {
        os_ << ' ' << val;
    }
    ~OutputFrame() {
        os_ << '\n';
        // add metadata etc, sure, why not
    }
private:
    std::ostream& os_;
};
// }}}
// table {{{
class ColumnHandle;
class Table {
public:
    Table(Metadata metadata0, vector<IntColumn::ptr> columns0)
            : md_(move(metadata0)), columns_(move(columns0)) {
        massert2(md_.columns_count() == isize(columns_));
    }
public:
    static Table read(InputFrame& frame);
    void write(const vector<ColumnHandle>& columns, const vector<RowNumbers>& rows, OutputFrame& frame);
    void write(const cnames& names, const vector<RowNumbers>& rows, OutputFrame& frame);

    i64 rows_count() const { return columns_[0]->rows_count(); }
    RowRange row_range() const { return RowRange(0, rows_count() - 1); }
    const IntColumn::ptr& column(const cname& name) const {
        return columns_[md_.column_id(name)];
    }
    const IntColumn::ptr& column(const index_t& column_id) const {
        bound_assert(column_id, columns_);
        return columns_[column_id];
    }
    const Metadata& metadata() const noexcept { return md_; }

    index_t column_id(const cname& name) const { return md_.column_id(name); }
    IntRange key_columns() const { return md_.key_columns(); }
    i64 columns_count() const { return md_.columns_count(); }
    IntRange columns() const { return md_.columns(); }
private:
    Metadata md_;
    // todo: rethink column metadata
    vector<IntColumn::ptr> columns_;
};
// }}}
// column handle {{{
class ColumnHandle {
public:
    ColumnHandle(Table const& tbl, cname name) :
        col_id_(tbl.column_id(name)), col_(*tbl.column(col_id_)) {}
    ColumnHandle(Table const& tbl, i64 column_id) :
        col_id_(column_id), col_(*tbl.column(col_id_)) {}
    const IntColumn& ref() const { return col_; }
    index_t id() const { return col_id_; }
    string _repr() const { return make_repr("ColumnHandle", {"column"}, col_.get()); }
private:
    index_t col_id_;
    const IntColumn::ref col_;
};
// }}}
// read/write {{{
void Table::write(const vector<ColumnHandle>& columns, const vector<RowNumbers>& rows, OutputFrame& frame) {
    // todo un-lazy it
    vstr names;
    for (const auto& col : columns)
        names.push_back(col.ref().name());
    write(names, rows, frame);
}
Table Table::read(InputFrame& frame) {
    auto md = frame.get_metadata();
    vector<IntColumn::ptr> columns(md.columns_count());
    for (auto const i : md.columns()) {
        columns[i] = IntColumn::make();
        columns[i]->name() = md.column_name(i);
    }
    auto column_it = columns.begin();
    for (auto elem = *frame; !frame.end(); elem = *(++frame)) {
        (*column_it)->push_back(elem);
        if (++column_it == columns.end()) column_it = columns.begin();
    }
    table_check(column_it == columns.begin(), "couldn't read the same number of values for each column");
    Table tbl(move(md), move(columns));
    dprintln("rows:", tbl.rows_count(), "columns:", tbl.columns_count());
    return tbl;
}
void Table::write(const cnames& names, const vector<RowNumbers>& rows, OutputFrame& frame) {
    frame.add_header(names);
    auto const columns = md_.column_ids(names);
    auto const columns_count = isize(columns);
    massert(columns_count > 0, "can't select 0 columns");
    RowNumbers::foreach(rows,
            [&frame, &columns_ = columns_, columns_count, &columns]
            (i64 row_num) {
        frame.new_row(columns_[columns[0]]->at(row_num));
        for (auto const i : IntRange(1, columns_count))
            frame.add_to_row(columns_[columns[i]]->at(row_num));
    });
}
// }}}
class ColumnPredicate { // {{{
public:
    ColumnPredicate(ColumnHandle h, vector<ValueInterval> intervals)
        : col_(h), intervals_(intervals) {}
    template <class AddResult>
    void filter(vector<RowRange> const& rows, AddResult add_result) const {
        for (RowRange const& range : rows)
            for (ValueInterval const& values : intervals_)
                add_result(col_.ref().equal_range(range, values), values);
    }
    bool match_row_id(const index_t& idx) const noexcept {
        auto const& value = col_.ref().at(idx);
//        auto const it = std::upper_bound(intervals_.begin(), intervals_.end(), value,
//                [](value_t const& l, ValueInterval const& r) {
//                    return !r.contains(l) && (r.r_infinity() || l < r.r());
//                });
//
//        return !intervals_.empty() && intervals_.front().contains(value);
        for (auto const& interval : intervals_)
            if (interval.contains(value))
                return true;
        return false;
    }
    string _repr() const { return make_repr("ColumnPredicate", {"column", "intervals"}, col_, intervals_); }
private:
    const ColumnHandle col_;
    vector<ValueInterval> intervals_;
}; // }}}
// query {{{
using columns_t = vector<ColumnHandle>;
// after info about key and column order was used
struct FullscanRequest {
    FullscanRequest(RowRange range, i64 fcol) : rows(range), first_column(fcol) {}
    FullscanRequest(FullscanRequest const&) = default;
    FullscanRequest(FullscanRequest &&) = default;
    FullscanRequest & operator=(FullscanRequest &&) = default;
    FullscanRequest & operator=(FullscanRequest const&) = default;
    bool operator<(const FullscanRequest& other) const { return rows < other.rows; }
    RowRange rows;
    i64 first_column;
    string _repr() const { return make_repr("FullscanRequest", {"rows", "first_column"}, rows, first_column); }
    string _str() const { return "(first_remaining_column=" + str(first_column) + ", rows=" + str(rows) + ")"; }
};
class AfterRangeScan {
public:
    AfterRangeScan(vector<FullscanRequest> fullscan_requests_0,
            vector<RowRange> remaining_rangescan_requests_0,
            i64 key_len)
        : fullscan_requests_(fun::sorted(fun::merge(fullscan_requests_0,
            fun::map(remaining_rangescan_requests_0, [&](auto const& r)
                { return FullscanRequest{r, key_len}; }))))
    {
    }
    vector<FullscanRequest> const& fullscan_requests() const noexcept { return fullscan_requests_; }
    string _repr() const {
        return make_repr("AfterRangeScan", {"fullscan_requests"}, fullscan_requests_);
    }
    string _str() const {
        return str(fullscan_requests_);
    }
private:
    vector<FullscanRequest> fullscan_requests_;
};
class AfterFullscan {
public:
    AfterFullscan(vector<RowNumbers> rows_0) noexcept : rows_(move(rows_0)) {}
    vector<RowNumbers> const& rows() const noexcept { return rows_; }
    string _repr() const { return make_repr("AfterFullscan", {"rows"}, rows_); }
    string _str() const { return _repr(); }
private:
    vector<RowNumbers> rows_;
};
// }}}
// {{{ table predicate
class TablePredicate {
public:
    TablePredicate(Metadata const& md, vector<ColumnPredicate> && preds_0) : md_(md), preds_(preds_0) {}
    AfterRangeScan perform_range_scan(RowRange const& rows) const {
        vector<FullscanRequest> not_scanned;
        vector<RowRange> rows_to_rangescan = {rows};
        vector<RowRange> rows_to_rangescan_rotate = {};
        for (const i64 c : md_.key_columns()) {
            if (rows_to_rangescan.empty())
                break;
            log_plan("Range scan for column:", str(c), "rows:", str(rows_to_rangescan));
            auto& pred = preds_[c];
            auto result_handler = [&] (RowRange r, ValueInterval const& v) {
                    if (v.is_single_value())
                        rows_to_rangescan_rotate.push_back(r);
                    else
                        not_scanned.emplace_back(r, c + 1);
                };
            pred.filter(rows_to_rangescan, result_handler);
            std::swap(rows_to_rangescan, rows_to_rangescan_rotate);
            rows_to_rangescan_rotate.clear();
        }

        return AfterRangeScan(move(not_scanned), move(rows_to_rangescan), md_.key_len());
    }
    RowNumbers perform_full_scan(RowRange const& rows, IntRange const& columns) const {
        RowNumbers row_numbers(rows);
        {
            RowNumbersEraser eraser(row_numbers); // todo: eraser -> builder
            for (auto const i : row_numbers.as_range()) {
                bool can_stay = true;
                for (auto const c : columns)
                    can_stay &= preds_[c].match_row_id(i);
                if (can_stay)
                    eraser.keep(i);
            }
        }
        return row_numbers;
    }
    vector<RowNumbers> perform_full_scan(vector<FullscanRequest> const& requests) const {
        vector<RowNumbers> outp;
        for (auto const& request : requests)
            outp.push_back(perform_full_scan(request.rows,
                           IntRange(request.first_column, md_.columns_count())));
        return outp;
    }
    string _repr() const {
        auto s = "TablePredicate(\n"s;
        for (auto const& pred : preds_)
            s += "    " + repr(pred) + "\n";
        return s + ')';
    }
private:
    Metadata const& md_;
    vector<ColumnPredicate> preds_;
};
struct Query {
    TablePredicate where_pred;
    columns_t select_cols;
    string _repr() const { return make_repr("Query", {"where_preds", "select_cols"}, where_pred, select_cols); }
};
// }}}
class TablePlayground { // {{{
public:
    TablePlayground(Table & table) : table_(table) {}
    void run(Query const& q, OutputFrame & outp) {
        auto const after_range = q.where_pred.perform_range_scan(table_.row_range());
        for (auto const& after_range_elem : after_range.fullscan_requests())
            log_plan("Range scan result:", str(after_range_elem));
        auto const rows = q.where_pred.perform_full_scan(after_range.fullscan_requests());
        log_plan("Full scan result:", str(rows));
        table_.write(q.select_cols, rows, outp);
    }
    void validate() const {
        vi64 prev_value;
        for (auto const i : table_.row_range()) {
            vi64 current_value;
            for (auto const j : table_.key_columns())
                current_value.push_back(table_.column(j)->at(i));
            table_check(i == 0 || !vector_less(current_value, prev_value),
                "key of row",  i, "is lesser than previous row");
            prev_value = move(current_value);
        }
    }
private:
    Table& table_;
}; // }}}
class PredOp { // {{{
public:
    enum Op : char {op_less = '<', op_equal = '=', op_more = '>'};
    PredOp(const string& op) {
        if (op == "<") op_ = op_less;
        else if (op == "=") op_ = op_equal;
        else if (op == ">") op_ = op_more;
        else unreachable_assert("unknown operator string in PredOp constructor: " + op);
    }
    bool is_single_elem() const { return op_ == op_equal; }
    string _str() const { return _repr(); }
    string _repr() const { return string(1, static_cast<char>(op_)); }
private:
    Op op_;
}; // }}}
class SingleRangePred { // {{{
public:
    template <class T1, class T2>
    SingleRangePred(T1 && op_0, T2 && value_0) : op_(op_0), value_(string_view(value_0)) {}
    PredOp const& op() const noexcept { return op_; }
    ValueInterval const& value() const noexcept { return value_; }
    string _repr() const {
        return make_repr("SingleRangePred", {"op", "value"}, op_, value_);
    }
    string _str() const { return _repr(); }
private:
    PredOp op_;
    ValueInterval value_;
};
class RangePredBuilder {
public:
    RangePredBuilder(Metadata const& md_0, i64 column_id_0) : md_(md_0), column_id_(column_id_0) {}
    template <class ...Ts>
    void add_pred(Ts && ...ts) { single_preds_.emplace_back(ts...); }
    ColumnPredicate build(Table const& tbl) {
        auto intervals = organize(single_preds_);
        if (intervals.empty())
            intervals.push_back(ValueInterval(0, 0, true, true, true, true));
        return ColumnPredicate(ColumnHandle(tbl, column_id_), move(intervals));
    }
    static vector<ValueInterval> organize(vector<SingleRangePred> single_preds) {
        fun::sort(single_preds, [](auto const& left, auto const& right)
                { return left.value() < right.value(); });
        vector<ValueInterval> res;
        for (auto const& pred : single_preds) {
            auto const& interval = pred.value();
            if (!res.empty() && res.back().neighbors_right(interval))
                res.back().merge_right(interval);
            else
                res.push_back(interval);
        }
        dprintln(res);
        return res;
    }
private:
    Metadata const& md_;
    const i64 column_id_;
    vector<SingleRangePred> single_preds_;
};
class TablePredicateBuilder {
public:
    TablePredicateBuilder(const Metadata& md) : md_(md) {
        for (auto const i : md_.columns())
            preds_.emplace_back(md, i);
    }
    template <class ...Ts>
    void add_pred(cname const& col, Ts && ...ts) {
        preds_[md_.column_id(col)].add_pred(ts...);
    }
    TablePredicate build(Table const& tbl) {
        vector<ColumnPredicate> preds;
        for (auto pred : preds_)
            preds.push_back(pred.build(tbl));
        return TablePredicate(md_, move(preds));
    }
private:
    const Metadata& md_;
    vector<RangePredBuilder> preds_;
}; // }}}
// parse {{{
Query parse(const Table& tbl, const string line) {
    TablePredicateBuilder where_builder(tbl.metadata());
    columns_t select_builder;
    // todo: select_builder: use metadata instead of table
    std::stringstream ss(line);
    string token;
    query_format_check(!ss.eof(), "empty line");
    ss >> token;
    query_format_check(token == "select", "no select at the beginning");
    {
        bool no_comma = true;
        while (no_comma && !ss.eof()) {
            ss >> token;
            if (token.back() != ',')
                no_comma = false;
            else
                token.pop_back();
            if (token != "*")
                select_builder.emplace_back(tbl, token);
            else
                for (auto const& id : tbl.metadata().columns())
                    select_builder.emplace_back(tbl, tbl.metadata().column_name(id));
        }
        query_format_check(!select_builder.empty(), "select list empty");
        query_format_check(!no_comma, "no comma after select list");
    }
    if (ss.eof()) {
        return Query{where_builder.build(tbl), move(select_builder)};
    }
    ss >> token;
    query_format_check(token == "where", "something else than 'where' after select list: " + token);
    {
        bool where_non_empty = false;
        bool no_comma = true;
        while (no_comma && !ss.eof()) {
            where_non_empty = true;
            ss >> token;
            if (token.back() != ',')
                no_comma = false;
            else
                token.pop_back();
            const auto sep_pos = token.find_first_of("=<>");
            query_format_check(sep_pos != string::npos, "<>= not found");
            const auto col = token.substr(0, sep_pos);
            const auto sep = PredOp(string(1, token[sep_pos]));
            const auto val = token.substr(sep_pos + 1, isize(token) - sep_pos - 1);
            where_builder.add_pred(col, sep, val);
        }
        query_format_check(where_non_empty, "where list empty");
        query_format_check(!no_comma, "no comma after where list");
    }
    if (ss.eof()) {
        return Query{where_builder.build(tbl), move(select_builder)};
    }
    ss >> token;
    throw query_format_error("there's something after 'where': " + token);
}
// parse }}}
// main loop {{{
struct CmdArgs {
    string filename;
};
void main_loop(const CmdArgs& args) {
    std::ifstream ifs(args.filename);
    table_check(!ifs.fail(), "couldn't open database file " + args.filename);
    InputFrame file(ifs);
    auto tbl = Table::read(file);
    TablePlayground t(tbl);
#ifndef NO_VALIDATION
    t.validate();
#endif
    string line;
    i64 count = 0;
    while (std::getline(std::cin, line)) {
        try {
            auto q = parse(tbl, line);
            Measure mes(str(++count));
            log_info("query:", line);
            dprintln(repr(q));
            println("query number:", count);
            OutputFrame outp(std::cout);
            t.run(q, outp);
            dprintln();
        } catch (const data_error& e) {
            dprintln();
            println("query error:", std::string(e.what()));
        }
    }
}
// }}}
// cmdline {{{
void quit(std::string msg) {
    log_info(msg, "- exiting");
    exit(13);
}
CmdArgs validate(int argc, char** argv) {
    if (argc != 2)
        quit("one argument required: path to the csv file, tab-separated, with \"-escape character");
    return {std::string(argv[1])};
}
int main(int argc, char** argv) {
    main_loop(validate(argc, argv));
}
// }}}
