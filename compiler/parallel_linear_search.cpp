// tests for parallel linear search

#define BOOST_TEST_MODULE parallel linear search
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

namespace bdata = boost::unit_test::data;

#include <thread>
#include <chrono>

#include "parallel_linear_search.hpp"
#include "ostream_mt.hpp"
#include "print.hpp"

const std::size_t num_workers = 64;

using namespace multithreading;

struct job {
    double duration;
    bool result;

    bool operator()(std::size_t idx) {
        (void) idx;
        std::this_thread::sleep_for(std::chrono::duration<double>(duration * 1e-3));
        if (idx % 200 == 0)
            print(cout_mt()).ln("done ", idx);
        return result;
    }
};

BOOST_AUTO_TEST_CASE(case0_func_det_single_end)
{
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i == 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 1590);
}

BOOST_AUTO_TEST_CASE(case1_func_det_multi_end)
{
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i >= 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 1590);
}

BOOST_AUTO_TEST_CASE(case2_func_det_none)
{
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, false});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST(!result);
}

BOOST_AUTO_TEST_CASE(case3_func_det_single_mid)
{
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i == 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 800);
}

BOOST_AUTO_TEST_CASE(case4_func_det_multi_end)
{
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i >= 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 800);
}


BOOST_AUTO_TEST_CASE(case5_func_rand_single_end)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };

    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i == 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 1590);
}

BOOST_AUTO_TEST_CASE(case6_func_rand_multi_end)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };

    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i >= 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 1590);
}

BOOST_AUTO_TEST_CASE(case7_func_rand_none)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, false});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST(!result);
}

BOOST_AUTO_TEST_CASE(case8_func_rand_single_mid)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i == 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 800);
}

BOOST_AUTO_TEST_CASE(case9_func_rand_multi_mid)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
    ParallelLinearSearch pls(num_workers);

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i >= 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->index == 800);
}
