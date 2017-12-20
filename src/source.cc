#include <bits/stdc++.h>
#include "basic.h"
#include "metaclass.h"
#include "defs.h"
#include "measure.h"

// {{{ exceptions and checks definitions
#define exception_check(ERROR_NAME, COND, MSG) do { if (!(COND)) throw ERROR_NAME(MSG); } while(0)
#define add_exception(ERROR_NAME, PARENT) struct ERROR_NAME : public PARENT { ERROR_NAME(std::string error) : PARENT(error) {} }

add_exception(data_error, std::runtime_error);
add_exception(query_format_error, data_error);
#define query_format_check(COND, MSG) exception_check(query_format_error, COND, MSG)
add_exception(table_error, data_error);
#define table_check(COND, MSG) exception_check(table_error, COND, MSG)
add_exception(query_semantics_error, data_error);
#define query_semantics_check(COND, MSG) exception_check(query_semantics_error, COND, MSG)
#undef add_exception
// }}}

class RowRange { // {{{
friend class RowNumbers;
friend class RowNumbersIterator;
public:
    RowRange() = default;
    RowRange(index_t l0, index_t r0) : l_(l0), r_(r0) {}
    bool operator==(const RowRange& other) const {
        return l_ == other.l_ && r_ == other.r_;
    }
    index_t l() const { return l_; }
    index_t& l() { return l_; }
    index_t r() const { return r_; }
    index_t& r() { return r_; }
    i64 len() const { return r_ - l_ + 1; }
    void normalize() { l_ = std::min(l_, r_ + 1); }
    auto _repr() const { return make_repr("RowRange", {"l", "r"}, l_, r_); }
private:
    index_t l_, r_;
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
    // todo implement
    RowRange equal_range(const RowRange& rng, value_t vall, value_t valr) const noexcept;
    RowRange equal_range(const RowRange& rng, value_t val) const noexcept {
        auto r = std::equal_range(data_.begin() + rng.l(), data_.begin() + rng.r() + 1, val);
        return RowRange(r.first - data_.begin(), r.second - data_.begin() - 1);
    }
    auto _repr() const { return make_repr("IntColumn", {"name", "length"}, name_, isize(data_)); }
private:
    std::vector<value_t> data_;
    cname name_;
}; // }}}
class RowNumbers {
public:
    RowNumbers(index_t len) : range_(0, len - 1), indices_set_used_(false) {}
    RowRange full_range() const {
        massert(!indices_set_used_, "for now, full_range() shouldn't be used with indices_set_used_");
        return range_;
    }
    template <typename F>
    void foreach(const F& f) const {
        if (indices_set_used_)
            for (const auto& row_id : indices_)
                f(row_id);
        else
            for (i64 row_id = range_.l(); row_id <= range_.r(); row_id++)
                f(row_id);
    }
    void narrow(RowRange narrower) {
        massert(!indices_set_used_, "indices_set_ used during narrowing");
        range_.l() = std::max(range_.l(), narrower.l());
        range_.r() = std::min(range_.r(), narrower.r());
        range_.normalize();
    }
    void set_indices_set(indices_t&& indices) {
        indices_set_used_ = true;
        indices_ = indices;
    }
private:
    indices_t indices_;
    RowRange range_;
    bool indices_set_used_;
}; // }}}

// metadata {{{
class Metadata {
public:
    Metadata(vstr&& columns0, i64 key_len0) : columns_(columns0), key_len_(key_len0) {
        table_check(key_len_ <= isize(columns_), "key_len " + std::to_string(key_len_) + " exceeds header length " + std::to_string(isize(columns_)));
    }
    const auto& columns() const { return columns_; }
    const auto& key_len() const { return key_len_; }
    auto _repr() const { return make_repr("Metadata", {"columns", "key_len"}, columns_, key_len_); }
private:
    vstr columns_;
    i64 key_len_;
};
// }}}
// input, output frame {{{
class InputFrame {
public:
    InputFrame(std::istream& is) : is_(is) {
    }
    Metadata get_metadata() {
        char sep = '\0';
        vstr header;
        i64 key_len = 0;
        bool metadata_found = false;
        while (sep != '\n' && !metadata_found) {
            std::string s;
            is_ >> s;
            table_check(!is_.eof(), "empty header");
            if (s.back() == ';') {
                s.pop_back();
                metadata_found = true;
            }
            header.push_back(s);
            is_.read(&sep, 1);
            table_check(!(sep != '\n' && sep != '\t' && sep != ' ' && sep != ';'),
                    "Bad separator: ascii code " + std::to_string(static_cast<int>(sep)));
        }
        if (metadata_found)
            is_ >> key_len;
        ++(*this);
        auto m = Metadata(std::move(header), std::move(key_len));
        dprintln(repr(m));
        return m;
    }
    vstr get_header() {
        char sep = '\0';
        vstr header;
        u64 key_len = 0;
        while (sep != '\n' && sep != ';') {
            std::string s;
            is_ >> s;
            table_check(!is_.eof(), "empty header");
            header.push_back(s);
            is_.read(&sep, 1);
            table_check(!(sep != '\n' && sep != '\t' && sep != ' ' && sep != ';'),
                    "Bad separator: ascii code " + std::to_string(static_cast<int>(sep)));
        }
        if (sep == ';')
            is_ >> key_len;
        dprintln("header", header);
        ++(*this);
        return header;
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
    Table(Metadata metadata0, std::vector<IntColumn::ptr> columns0) : metadata_(std::move(metadata0)), columns_(std::move(columns0)) {}
public:
    static Table read(InputFrame& frame);
    void write(const std::vector<ColumnHandle>& columns, const RowNumbers& rows, OutputFrame& frame);
    void write(const cnames& names, const RowNumbers& rows, OutputFrame& frame);
    const IntColumn::ptr& column(const cname& name) const {
        return columns_[resolve_column_(name)];
    }
    const IntColumn::ptr& column(const index_t& column_id) const {
        massert(column_id < isize(columns_), "index out of bound: " + std::to_string(column_id));
        return columns_[column_id];
    }
    index_t column_id(const cname& name) const { return resolve_column_(name); }
    i64 rows_count() const { return columns_[0]->rows_count(); }
    i64 columns_count() const { return isize(columns_); }
    const Metadata& metadata() const noexcept { return metadata_; }
private:
    indices_t resolve_columns_(const cnames& names) const {
        indices_t columns;
        std::transform(names.begin(), names.end(), std::back_inserter(columns),
                [this](const auto& name){ return this->resolve_column_(name); });
        return columns;
    }
    index_t resolve_column_(const cname& name) const {
        for (i64 i = 0; i < isize(columns_); i++)
            if (columns_[i]->name() == name)
                return i;
        table_check(false, "unknown column name: " + name); 
    }
    Metadata metadata_;
    std::vector<IntColumn::ptr> columns_;
};
// }}}
// column handle {{{
class ColumnHandle {
public:
    ColumnHandle(const Table& tbl, std::string name) :
        col_id_(tbl.column_id(name)), col_(*tbl.column(col_id_))
    {
    }
    const IntColumn& ref() const { return col_; }
    index_t id() const { return col_id_; }
    auto _repr() const { return make_repr("ColumnHandle", {"column"}, col_.get()); }
private:
    index_t col_id_;
    const IntColumn::ref col_;
};
// }}}
// read/write {{{
void Table::write(const std::vector<ColumnHandle>& columns, const RowNumbers& rows, OutputFrame& frame) {
    // todo un-lazy it
    vstr names;
    for (const auto& col : columns)
        names.push_back(col.ref().name());
    write(names, rows, frame);
}
Table Table::read(InputFrame& frame) {
    auto metadata = frame.get_metadata();
    const auto& header = metadata.columns();
    std::vector<IntColumn::ptr> columns(metadata.columns().size());

    columns.resize(metadata.columns().size());
    for (i64 i = 0; i < isize(header); i++) {
        columns[i] = IntColumn::make();
        columns[i]->name() = header[i];
    }
    massert(!columns.empty(), "empty header");
    auto column_it = columns.begin();
    for (auto elem = *frame; !frame.end(); elem = *(++frame)) {
        (*column_it)->push_back(elem);
        if (++column_it == columns.end()) column_it = columns.begin();
    }
    table_check(column_it == columns.begin(), "couldn't read the same number of values for each column");
    Table tbl(std::move(metadata), std::move(columns));
    dprintln("rows:", tbl.rows_count(), "columns:", tbl.columns_count());
    return tbl;
}
void Table::write(const cnames& names, const RowNumbers& rows, OutputFrame& frame) {
    frame.add_header(names);
    const auto columns = resolve_columns_(names);
    const auto columns_count = isize(columns);
    // TODO add this check to "query" class
    massert(columns_count > 0, "can't select 0 columns");
    rows.foreach([&frame, &columns_ = columns_, columns_count, &columns](i64 row_num) {
        frame.new_row(columns_[columns[0]]->at(row_num));
        for (i64 i = 1; i < columns_count; i++)
            frame.add_to_row(columns_[columns[i]]->at(row_num));
    });
}
// }}}

// expression bindings {{{
class ConstBinding {
public:
    ConstBinding(value_t value0) : value_(value0) {}
add_field_with_lvalue_ref_accessor(value_t, value)
};
class ColumnBinding {
public:
    ColumnBinding(const IntColumn& col, const RowRange& rng) : col_(col), rng_(rng) {}
    const IntColumn& column() const { return col_; }
    const RowRange& row_range() const { return rng_; }
    template <typename T>
    auto equal_range(const T& value) const {
        return column().equal_range(row_range(), value);
    }
    auto _repr() const { return make_repr("ColumnBinding", {"column", "row_range"}, col_, rng_); }
private:
    const IntColumn& col_;
    const RowRange& rng_;
};
// }}}
class PredOp { // {{{
public:
    enum Op : char {op_less = '<', op_equal = '=', op_more = '>'};
    //todo: through the power of modern c++, let's change PredOp() and _repr() implementations,
    // so that we don't need to enumerate them by hand - let's use static mapping instead
    // ...alternatively, let's use BOOST_PREPROCESSOR to generate classes like this (my own create_class)
    
    PredOp(const std::string& op) {
        if (op == "<") op_ = op_less;
        else if (op == "=") op_ = op_equal;
        else if (op == ">") op_ = op_more;
        else unreachable_assert("unknown operator string in PredOp constructor: " + op);
    }
    bool is_single_elem() const { return op_ == op_equal; }
    auto _str() const { return this->_repr(); }
    std::string _repr() const {
        if (op_ == op_less) return "<";
        if (op_ == op_equal) return "=";
        if (op_ == op_more) return ">";
        unreachable_assert("unknown operator in PredOp: " + std::string(1, static_cast<char>(op_)));
    }
    RowRange filter(const ColumnBinding& left, const ConstBinding& right) const noexcept {
        auto result_range = left.equal_range(right.value());
        switch (op_) {
            case op_equal: return result_range;
            case op_less: return RowRange(left.row_range().l(), result_range.l() - 1ll);
            case op_more: return RowRange(result_range.r() + 1ll, left.row_range().r());
        }
        unreachable_assert("unknown pred: " + repr(op_));
    }
    bool match(const ConstBinding& left, const ConstBinding& right) const noexcept {
        switch (op_) {
            case op_less: return left.value() < right.value();
            case op_equal: return left.value() == right.value();
            case op_more: return left.value() > right.value();
        }
        unreachable_assert("unknown pred: " + repr(op_));
    }
private:
    Op op_;
}; // }}}
class ColumnPredicate { // {{{
public:
    using ref = std::reference_wrapper<ColumnPredicate>;
    using ptr = std::unique_ptr<ColumnPredicate>;
    template <typename ...Ts>
    static ptr make(const Ts&... ts) { return std::make_unique<ColumnPredicate>(ts...); }
    ColumnPredicate(const ColumnHandle& col0, PredOp pred0, value_t right_arg0)
        : col_(col0), pred_(pred0), right_arg_(right_arg0) 
    {
    }
    RowRange filter(const RowRange& rng) const noexcept {
        ColumnBinding left_arg(col_.ref(), rng);
        return pred_.filter(left_arg, right_arg_);
    }
    void filter(RowNumbers& rows) const noexcept {
        rows.narrow(filter(rows.full_range()));
    }
    bool match_value(const value_t& val) const noexcept {
        return pred_.match(val, right_arg_);
    }
    bool match_row_id(const index_t& idx) const noexcept {
        return match_value(col_.ref().at(idx));
    }
    bool can_be_range_filtered() const noexcept {
        return true; // can be false for e.g. LIKE predicate for std::string
    }
    const ColumnHandle& column() const { return col_; }
    const PredOp& pred() const { return pred_; }
    auto _repr() const { return make_repr("ColumnPredicate", {"column", "predicate", "right_arg"}, col_, pred_, right_arg_.value()); }
    auto _str() const { return col_.ref().name() + str(pred_) + str(right_arg_.value()); }
private:
    const ColumnHandle col_;
    const PredOp pred_;
    ConstBinding right_arg_;
}; // }}}

// query {{{
using preds_t = std::vector<ColumnPredicate::ptr>;
using pred_groups_t = std::vector<std::vector<ColumnPredicate::ref>>;
using columns_t = std::vector<ColumnHandle>;

struct Query {
    preds_t where_preds;
    columns_t select_cols;
    auto _repr() const { return make_repr("Query", {"where_preds", "select_cols"}, where_preds, select_cols); }
}; // }}}

class TablePlayground { // {{{
    static index_t get_first_fullscan_column_id_(index_t key_len, const pred_groups_t& preds) {
        for (i64 i = 0; i < key_len; i++) {
            bool can_be_range_filtered = true;
            bool matches_single_element = false;
            for (const auto& pred : preds[i]) {
                if (pred.get().pred().is_single_elem())
                    matches_single_element = true;
                if (!pred.get().can_be_range_filtered())
                    can_be_range_filtered = false;
            } 
            if (!can_be_range_filtered)
                return i;
            if (!matches_single_element)
                return i + 1;
        }
        return key_len;
    }
    static pred_groups_t group_preds_by_column_(const preds_t& where_preds, index_t columns_count) {
        pred_groups_t preds(columns_count);
        for (i64 i = 0; i < isize(where_preds); i++) {
            auto& pred = where_preds[i];
            massert2(pred->column().id() < isize(preds));
            preds[pred->column().id()].push_back(*pred);
        }
        return preds;
    }
    template <typename It>
    static void narrow_by_range_(RowNumbers& rows, const It preds_begin, const It preds_end) {
        for (auto it = preds_begin; it != preds_end; it++)
            for (const auto& pred : *it)
                pred.get().filter(rows);
    }
    template <typename It>
    static bool has_any_predicates(const It preds_begin, const It preds_end) {
        for (auto it = preds_begin; it != preds_end; it++)
            if (!it->empty())
                return true;
        return false;
    }
    template <typename It>
    static void narrow_by_scan(RowNumbers& rows, const It preds_begin, const It preds_end) {
        if (!has_any_predicates(preds_begin, preds_end))
            return;
        std::vector<index_t> remaining;
        rows.foreach([&preds_begin, &preds_end, &remaining](i64 row_id) {
            bool remains = true;
            for (auto it = preds_begin; it != preds_end; it++)
                for (const auto& pred : *it)
                    remains &= pred.get().match_row_id(row_id);
            if (remains)
                remaining.push_back(row_id);
        });
        rows.set_indices_set(std::move(remaining));
    }
    RowNumbers perform_where_(const preds_t& where_preds) {
        RowNumbers rows(table_.rows_count());
        const auto preds = group_preds_by_column_(where_preds, table_.columns_count());
        const auto first_fullscan_column_id = get_first_fullscan_column_id_(table_.metadata().key_len(), preds);
        log_info("range scan for columns 0.." + std::to_string(first_fullscan_column_id - 1));
        narrow_by_range_(rows, preds.begin(), preds.begin() + first_fullscan_column_id);
        log_info("full scan for remaining rows: " + repr(rows.full_range()));
        narrow_by_scan(rows, preds.begin() + first_fullscan_column_id, preds.end());
        return rows;
    }
    void perform_select_(const columns_t& select_cols, const RowNumbers& rows, OutputFrame& outp) const {
        table_.write(select_cols, rows, outp);
    }
public:
    TablePlayground(Table& table) : table_(table) {}
    void run(const Query& q, OutputFrame& outp) {
        auto row_numbers = perform_where_(q.where_preds);
        perform_select_(q.select_cols, row_numbers, outp);
    }
    void validate() const {
        const auto len = table_.metadata().key_len();
        vi64 prev_value;
        for (i64 i = 0; i < table_.rows_count(); i++) {
            vi64 current_value;
            for (i64 j = 0; j < len; j++)
                current_value.push_back(table_.column(j)->at(i));
            if (i != 0)
                table_check(!vector_less(current_value, prev_value), 
                    "row " + std::to_string(i) + "'s key is lesser than previous row");
            prev_value = std::move(current_value);
        }
    }
private:
    Table& table_;
}; // }}}

// parse {{{
Query parse(const Table& tbl, const std::string line) {
    Query q;
    std::stringstream ss(line);
    std::string token;
    query_format_check(!ss.eof(), "empty line");
    ss >> token;
    query_format_check(token == "select", "no select at the beginning");
    dprint("<select>");
    {
        bool no_comma = true;
        while (no_comma && !ss.eof()) {
            ss >> token;
            if (token.back() != ',')
                no_comma = false;
            else
                token.pop_back();
            dprint(" [" + token + "]");
            q.select_cols.emplace_back(tbl, token);
        }
        query_format_check(!q.select_cols.empty(), "select list empty");
        query_format_check(!no_comma, "no comma after select list");
    }
    if (ss.eof()) {
        dprintln(';');
        return q;
    }
    ss >> token;
    query_format_check(token == "where", "something else than 'where' after select list: " + token);
    dprint(" <where>");
    {
        bool no_comma = true;
        while (no_comma && !ss.eof()) {
            ss >> token;
            if (token.back() != ',')
                no_comma = false;
            else
                token.pop_back();

            // todo: support (a>3 || a=0) && (b=13) && (c>14) && (c>2) && (c<10)
            // next will be >=, <=
            // next let's try to find a general solution

            const auto sep = token.find_first_of("=<>");
            query_format_check(sep != std::string::npos, "<>= not found");
            const auto sep_val = PredOp(std::string(1, token[sep]));
            const auto col = token.substr(0, sep);
            const auto val = token.substr(sep + 1, isize(token) - sep - 1);
            q.where_preds.push_back(ColumnPredicate::make(ColumnHandle(tbl, col), sep_val, stoll(val)));
            dprint(" " + str(*q.where_preds.back()));
        }
        query_format_check(!q.where_preds.empty(), "where list empty");
        query_format_check(!no_comma, "no comma after where list");
    }
    if (ss.eof()) {
        dprintln(";");
        return q;
    }
    ss >> token;
    query_format_check(false, "there's something after 'where': " + token);
}
// parse }}}
// main loop {{{
struct CmdArgs {
    std::string filename;
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
    std::string line;
    i64 count = 0;
    while (std::getline(std::cin, line)) {
        try {
            println();
            auto q = parse(tbl, line);
            println("query:", line);
            log_info("query:", line);
            dprintln(repr(q));
            Measure mes(std::to_string(++count));
            using namespace std::chrono_literals;
            OutputFrame outp(std::cout);
            t.run(q, outp);
            dprintln();
        } catch (const data_error& e) {
            dprintln();
            println("query error:", e.what());
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
