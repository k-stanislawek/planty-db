#include "basic.h"

template <typename IntType = i64>
class IntIterator {
    friend class IntRange;
public:
    IntIterator(IntType start, IntType step = 1) : p_(start), step_(step) {}
    IntType const& operator*() const noexcept { return p_; }
    IntIterator& operator++() noexcept { p_ += step_; return *this; }
    bool operator==(IntIterator const& other) const noexcept { return p_ == other.p_; }
    bool operator!=(IntIterator const& other) const noexcept { return p_ != other.p_; }
private:
    IntType p_;
    const IntType step_;
};
using IntType = i64;
class IntRange {
public:
    using iterator = IntIterator<IntType>;
    IntRange(IntType start, IntType stop, IntType step = 1) 
        : begin_(start, step), end_(((stop - start - 1) / step + 1) * step + start, step) {
        massert2(start == stop || (stop - start) * step > 0);
    }
    iterator begin() const { return begin_; }
    iterator end() const { return end_; }
    auto _repr() const { return "IntRange(" + str(begin_.p_) + "," + str(end_.p_) + "," + str(begin_.step_) + ")"; }
private:
    const iterator begin_;
    const iterator end_;
};
