#pragma once
#include "defs.h"

class Measure {
public:
    Measure(std::string name) : name_(name), start_(std::chrono::high_resolution_clock::now())
    {
    }
    ~Measure() {
        auto finish = std::chrono::high_resolution_clock::now();
        if (std::uncaught_exception())
            return;
        log_perf(name_, (finish - start_).count());
    }
private:
    std::string name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};
