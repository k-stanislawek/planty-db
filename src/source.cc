#include <bits/stdc++.h>
#include "basic.h"

using index_t = i64;
using value_t = i64;
using vstr = std::vector<std::string>;
using indices_t = std::vector<index_t>;
using vi64 = std::vector<i64>;
using cname = std::string;
using cnames = std::vector<std::string>;
// note: some unique "column id" would be very useful, because we don't want Columns to have references to Table,
// and still we want to differentiate columns with the same name
// for now, we use cname for id's purposes

class RowRange {
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
    size_t size() const { return static_cast<size_t>(r_ - l_ + 1); }
    void normalize() { l_ = std::min(l_, r_ + 1); }
private:
    index_t l_, r_;
};

class IntColumn {
public:
    using ptr = std::unique_ptr<IntColumn>;
    static ptr make() { return std::make_unique<IntColumn>(); }
    void push_back(const value_t elem) { data_.push_back(elem); }
    cname& name() noexcept { return name_; }
    const cname& name() const noexcept { return name_; }
    const value_t& ref(index_t index) const { return data_.at(index); } // todo add noexcept, at replace with [] and assert
    index_t count() const noexcept { return isize(data_); }
    // todo implement
    RowRange equal_range(value_t vall, value_t valr) const noexcept;
    RowRange equal_range(value_t val) const noexcept {
        auto r = std::equal_range(data_.begin(), data_.end(), val);
        return RowRange(r.first - data_.begin(), r.second - data_.begin() - 1);
    }
private:
    std::vector<value_t> data_;
    cname name_;
};

namespace pred { // {{{
template <class Pred> class Not;
template <class ...Ps> class And;
template <class ...Ps> class Or;
class IntEq;
class IntLess;
class IntRange;
class IntLessEq;
} // }}}

//class RowNumbers;
class RowNumbersIterator {
public:
    RowNumbersIterator() = default;
    RowNumbersIterator(bool is_range, indices_t::const_iterator indices_it, RowRange range)
        : indices_it_(indices_it), range_(range), is_range_(is_range)
    {}
    bool operator==(const RowNumbersIterator& other) const {
        return is_range_ ? (range_ == other.range_) : indices_it_ == other.indices_it_;
    }
    bool operator!=(const RowNumbersIterator& other) const {
        return !(*this == other);
    }
    RowNumbersIterator& operator++() {
        if (is_range_)
            ++range_.l();
        else
            ++indices_it_;
        return *this;
    }
    index_t operator*() const {
        if (is_range_)
            return range_.l();
        else
            return *indices_it_;
    }
    RowNumbersIterator make_end() const {
        return RowNumbersIterator(is_range_, indices_it_, RowRange(range_.r() + 1, range_.r()));
    }
private:
    indices_t::const_iterator indices_it_;
    RowRange range_;
    const bool is_range_;
};

class RowNumbers {
public:
    using iterator = RowNumbersIterator;
    RowNumbers(index_t len) : range_(0, len - 1) {}
    RowRange full_range() const { return range_; }

    iterator begin() const {
        return RowNumbersIterator(!indices_set_used_, indices_.cbegin(), range_);
    }
    iterator end() const {
        return RowNumbersIterator(!indices_set_used_, indices_.cend(), RowRange(range_.r() + 1, range_.r()));
    }
    void narrow(RowRange narrower) {
        massert(!indices_set_used_, "indices_set_ used during narrowing");
        range_.l() = std::max(range_.l(), narrower.l());
        range_.r() = std::min(range_.r(), narrower.r());
        range_.normalize();
    }
    void set_indices_set(indices_t&& indices) { indices_ = indices; }
private:
    indices_t indices_;
    RowRange range_;
    bool indices_set_used_;
};

// input, output frame {{{zo
class InputFrame {
public:
    struct IntegrityError : public std::runtime_error { using std::runtime_error::runtime_error; };
    InputFrame(std::istream& is) : is_(is) {
    }
    vstr get_header() {
        char sep = '\0';
        vstr header;
        while (sep != '\n') {
            std::string s;
            is_ >> s;
            header.push_back(s);
            is_.read(&sep, 1);
            if (sep != '\n' && sep != '\t' && sep != ' ') {
                throw InputFrame::IntegrityError(std::string("Bad separator: ascii code ") + std::to_string(static_cast<int>(sep)));
            }
        }
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
class Table {
public:
    // read/write {{{
    void read(InputFrame& frame) {
        const vstr header = frame.get_header();
        columns_.resize(header.size());
        for (i64 i = 0; i < isize(header); i++) {
            columns_[i] = IntColumn::make();
            columns_[i]->name() = header[i];
        }
        massert(!columns_.empty(), "empty header");
        auto column_it = columns_.begin();
        for (auto elem = *frame; !frame.end(); elem = *(++frame)) {
            dprintln("elem", elem);
            (*column_it)->push_back(elem);
            if (++column_it == columns_.end()) column_it = columns_.begin();
        }
        if (column_it != columns_.begin())
            throw InputFrame::IntegrityError("File was incomplete");
#ifndef NDEBUG
        for (auto& column : columns_)
            massert(column->count() == columns_.front()->count(), "wrong column length: " + column->name());
#endif   
    }
    void write(const cnames& names, const RowNumbers& rows, OutputFrame& frame) {
        frame.add_header(names);
        const auto columns = resolve_columns_(names);
        const size_t columns_count = columns.size();
        for (const auto row_num : rows) {
            frame.new_row(columns_[columns[0]]->ref(row_num));
            for (u64 i = 1; i < columns_count; i++)
                frame.add_to_row(columns_[columns[i]]->ref(row_num));
        }
    }
    // }}}
    const IntColumn::ptr& column(const cname& name) const noexcept {
        return columns_.at(resolve_column_(name));
    }
    const IntColumn::ptr& column(const index_t& column_id) const noexcept {
        return columns_.at(column_id);
    }
    index_t column_id(const cname& name) const noexcept { return resolve_column_(name); }
    size_t count() const { return columns_.at(0)->count(); }
private:
    indices_t resolve_columns_(const cnames& names) const {
        indices_t columns;
        std::transform(names.begin(), names.end(), std::back_inserter(columns),
                [this](const auto& name){ return this->resolve_column_(name); });
        return columns;
    }
    index_t resolve_column_(const cname& name) const noexcept {
        for (i64 i = 0; i < isize(columns_); i++)
            if (columns_[i]->name() == name)
                return i;
        massert(false, "Unknown column name: " + name);
        return 0;
    }
    std::vector<std::string> header_key_;
    std::vector<IntColumn::ptr> columns_;
};

// main loop {{{
class TablePlayground {
public:
    TablePlayground(Table& table) : table_(table) {}
    void run(vstr& where_cols, vi64& where_vals, vstr& select_cols, OutputFrame& outp) {
        RowNumbers rows(table_.count());
        massert(where_cols.size() == where_vals.size(), "different cols and vals len");
        std::vector<std::vector<value_t>> filters(table_.count());
        for (i64 i = 0; i < isize(where_cols); i++) {
            const value_t value = where_vals.at(i);
            const auto column_id = table_.column_id(where_cols.at(i));
            filters[column_id].push_back(value);
        }
        i64 first_non_range_column_id = isize(filters);
        for (i64 column_id = 0; column_id < isize(filters); column_id++) {
            const auto& column = table_.column(column_id);
            if (filters[column_id].empty()) {
                first_non_range_column_id = column_id;
                break;
            }
            for (const auto value : filters[column_id])
                rows.narrow(column->equal_range(value));
        }
        {
            for (auto& values : filters)
                std::sort(values.begin(), values.end());
            std::vector<index_t> remaining;
            for (const auto value : rows) {
                bool remains = true;
                for (i64 column_id = first_non_range_column_id; column_id < isize(filters); column_id++)
                    remains &= filters[column_id].empty() || std::binary_search(filters[column_id].begin(), filters[column_id].end(), table_.column(column_id)->ref(value));
                if (remains)
                    remaining.push_back(value);
            }
            rows.set_indices_set(std::move(remaining));
        }
        table_.write(select_cols, rows, outp);
    }
private:
    Table& table_;
};
struct CmdArgs {
    std::string filename;
};
void main_loop(const CmdArgs& args) {
    Table tbl;
    std::ifstream ifs(args.filename);
    InputFrame file(ifs);
    tbl.read(file);
    TablePlayground t(tbl);
    while (true) {
        dprintln("k");
        auto where_cols = readv<std::string>(read<u64>());
        dprintln("l");
        auto where_vals = readv<value_t>(where_cols.size());
        dprintln("m");
        auto select_cols = readv<std::string>(read<u64>());
        OutputFrame outp(std::cout);
        dprintln(where_cols, where_vals, select_cols);
        t.run(where_cols, where_vals, select_cols, outp);
    }
}
// }}}
// cmdline {{{
void quit(std::string msg) {
    lprintln(msg, "- exiting");
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
