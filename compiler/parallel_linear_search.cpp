// tests for parallel linear search

#include "parallel_linear_search.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include "ostream_mt.hpp"

using namespace multithreading;

int main() {
    ParallelLinearSearch pls(64);
    auto job = [](std::size_t duration, bool result) {
        return [=](std::size_t idx) {
            (void) idx;
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
            return result;
        };
    };

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 2010; ++i) {
        pls.append_job(job(i, false && i >= 2000));
    }

    pls.end();

    auto r = pls.result();
    if (r) {
       cout_mt() << "success idx = " << r->first << std::endl;
    }
    else {
        cout_mt() << "failure\n";
    }


    return 0;
}
