#pragma once
#include <bits/stdc++.h>
#include "basic.h"

#define add_field_with_lvalue_ref_accessor(TYPE, NAME) \
public: \
    TYPE& NAME() { return NAME##_; } \
    TYPE const& NAME() const { return NAME##_; } \
private: \
    TYPE NAME##_;

