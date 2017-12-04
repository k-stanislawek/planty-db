#include <bits/stdc++.h>
#include "basic.h"

using index_t = int64_t;

class Column {
public:
    using ptr = std::unique_ptr<Column>;
    virtual void push_back(const std::string& token) = 0;
    void set_name(const std::string& name) { name_ = name; }
    std::string get_name() const { return name_; }
    virtual std::string get_value(index_t index) const = 0;
    virtual index_t count() const = 0;
protected:
    std::string name_;
};

class IntColumn : public Column {
public:
    using data_type = int64_t;
    void push_back(const std::string& token) override {
        data_.push_back(std::stoll(token));
    }
    std::string get_value(index_t index) const override {
        massert(index >= 0 && index < isize(data_), "Index out of bound: " + std::to_string(index));
        return std::to_string(data_[index]);
    }
    const data_type& ref(index_t index) const { 
        massert(index >= 0 && index < isize(data_), "Index out of bound: " + std::to_string(index));
        return data_[index];
    }
    const std::vector<data_type>& ref() const {
        return data_;
    }
    index_t count() const override { return isize(data_); }

private:
    std::vector<data_type> data_;
};

class IntPredicate {
public:
    virtual bool fulfill(const IntColumn::data_type& value) const = 0;
};

template <typename T>
bool vector_less(const std::vector<T>& v1, const std::vector<T>& v2) {
    massert(v1.size() == v2.size(), "wrong value len");
    for (size_t i = 0; i < v1.size(); i++)
        if (v1[i] != v2[i])
            return v1[i] < v2[i];
    return false;
}

class IntRangePredicate {
public:
    IntRangePredicate(const std::vector<IntColumn::data_type>& vs) : vs_(vs) {
    }
private:
    friend bool operator<(const IntRangePredicate& pred, std::vector<IntColumn::data_type>& val) {
        return vector_less(pred.vs_, val);
    }
    friend bool operator<(std::vector<IntColumn::data_type>& val, const IntRangePredicate& pred) {
        return vector_less(val, pred.vs_);
    }
    friend bool operator==(std::vector<IntColumn::data_type>& val, const IntRangePredicate& pred) {
        return val == pred.vs_;
    }
    friend bool operator==(const IntRangePredicate& pred, std::vector<IntColumn::data_type>& val) {
        return val == pred.vs_;
    }
    const std::vector<IntColumn::data_type> vs_;
};

class TableView {
public:
    TableView(index_t len) : indices_(make_full_indices_(len)) {
    }
    TableView(std::vector<index_t> indices) : indices_(indices)  {
    }
    void filter(const IntColumn& col, const IntPredicate& pred) {
        std::remove_if(indices_.begin(), indices_.end(), [&pred, &col](index_t i) {
            return !pred.fulfill(col.ref(i));
        });
    }

    void filter_range(const IntColumn& col, const IntRangePredicate& pred) {
        // lower bound, upper bound
        auto begin_end = std::equal_range(indices_.begin(), indices_.end(),
            [&pred, &col](index_t a, index_t b) {
                return col.ref(a)
            });


    }

private:
    std::vector<index_t> make_full_indices_(index_t length) {
        std::vector<index_t> indices(length);
        for (index_t i = 0; i < isize(indices); i++)
            indices[i] = i;
        return indices;
    }
    std::vector<index_t> indices_;
};

class InputFile;
class Table {
public:
    Table(const InputFile& inp);

    std::pair<std::vector<size_t>, std::vector<size_t>> get_column_indices(const std::vector<std::string>& cols) const {
        std::vector<size_t> key, other;
        for (const auto& c : cols) {
            auto it = find(header_.begin(), header_.end(), c);
            massert(it != header_.end(), "bad column name: " + c);
            size_t i = it - header_.begin();
            if (i < key_length_)
                key.push_back(i);
            else
                other.push_back(i);
        }
        sort(key.begin(), key.end());
        size_t key_end;
        for (key_end = 0; key_end < key.size() && key_end == key[key_end]; key_end++);
        for (size_t i = key_end; i < key.size(); i++)
            other.push_back(key[i]);
        key.resize(key_end);
        return make_pair(std::move(key), std::move(other));
    }



private:
    std::vector<std::string> header_;
    size_t key_length_;
    std::vector<IntColumn::ptr> columns_;
};

class InputFile {
public:
    InputFile(std::string filename);
};

void quit(std::string msg) {
    lprintln(msg, "- exiting");
    exit(13);
}

int main(int argc, char** argv) {
    if (argc != 2)
        quit("one argument required: path to the csv file, tab-separated, with \"-escape character");
    const auto filename = std::string(argv[1]);
    std::ifstream file;
    file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
    try {
        file.open(filename);
    } catch (const std::ios_base::failure& e) {
        quit("Error opening file " + filename);
    }
    Table t = InputFile(filename);
    while (true) {
        size_t n, m;
        std::cin >> n >> m;
        std::vector<std::string> select(n), where(m);
        while (n--)
            std::cin >> select[n];
        while (m--)
            std::cin >> where[m];
        

    }
}
