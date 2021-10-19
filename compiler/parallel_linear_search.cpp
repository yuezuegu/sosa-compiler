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

using namespace multithreading;

const std::size_t num_workers = 64;

struct job {
    double duration;
    bool result;

    template <typename WorkerData>
    bool operator()(std::size_t idx, const WorkerData &worker_data) {
        (void) idx;
        (void) worker_data;
        std::this_thread::sleep_for(std::chrono::duration<double>(duration * 1e-3));
        if (idx % 200 == 0)
            print(cout_mt()).ln("done ", idx);
        return result;
    }
};

ParallelLinearSearch<job> pls(num_workers);

BOOST_AUTO_TEST_CASE(case0_func_det_single_end)
{
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i == 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 1590);
}

BOOST_AUTO_TEST_CASE(case1_func_det_multi_end)
{
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i >= 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 1590);
}

BOOST_AUTO_TEST_CASE(case2_func_det_none)
{
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
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i == 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 800);
}

BOOST_AUTO_TEST_CASE(case4_func_det_multi_end)
{
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i >= 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 800);
}


BOOST_AUTO_TEST_CASE(case5_func_rand_single_end)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i == 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 1590);
}

BOOST_AUTO_TEST_CASE(case6_func_rand_multi_end)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i >= 1590});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 1590);
}

BOOST_AUTO_TEST_CASE(case7_func_rand_none)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
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
    
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i == 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 800);
}

BOOST_AUTO_TEST_CASE(case9_func_rand_multi_mid)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{random() * 5 + i / 5, i >= 800});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 800);
}

BOOST_AUTO_TEST_CASE(case10_func_rand_0)
{
    auto rdgen = bdata::random();
    auto random = [&, it = rdgen.begin()]() mutable { auto v = *it; ++it; return v; };
    
    pls.begin();

    int expected = -1;

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        auto r = random() >= 0.5;
        pls.append_job(job{random() * 5 + i / 5, r});
        if (r && expected < 0)
            expected = i;
    }

    pls.end();

    print(cout_mt()).ln("expected ", expected);

    auto result = pls.result();

    BOOST_TEST((bool) result == (expected >= 0));
    if (expected > 0)
        BOOST_TEST(result->idx == expected);
}

BOOST_AUTO_TEST_CASE(case11_func_det_first)
{
    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{i == 0 ? 3.0 : 1.0, i == 0});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 0);
}

BOOST_AUTO_TEST_CASE(case12_func_det_last)
{

    pls.begin();

    for (int i = 0; pls.should_continue() && i < 1600; ++i) {
        pls.append_job(job{(double) i, i >= 1599});
    }

    pls.end();

    auto result = pls.result();

    BOOST_TEST((bool) result);
    BOOST_TEST(result->idx == 1599);
}
