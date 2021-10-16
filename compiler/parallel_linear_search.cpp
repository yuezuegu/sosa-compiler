// tests for parallel linear search

#include "parallel_linear_search.hpp"

#include <iostream>
#include <thread>
#include <chrono>

using namespace multithreading;

int main() {
    ParallelLinearSearch pls(8);
    auto job = [](std::size_t duration, bool result) {
        return [=](std::size_t) {
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
            return result;
        };
    };

    pls.append_job(job(5, false));
    pls.append_job(job(1, false));
    pls.append_job(job(100, true));
    pls.append_job(job(1, false));
    pls.append_job(job(5, true));
    pls.append_job(job(1, false));
    pls.append_job(job(1, false));
    pls.append_job(job(1, false));
    pls.append_job(job(5, true));
    pls.append_job(job(1, false));
    pls.append_job(job(1, false));
    pls.append_job(job(1, false));

    std::size_t idx;
    if (pls.execute(idx)) {
        std::cout << "success idx = " << idx << std::endl;
    }
    else {
        std::cout << "failure";
    }


    return 0;
}
