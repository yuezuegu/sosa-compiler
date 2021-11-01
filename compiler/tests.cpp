#define BOOST_TEST_MODULE compiler

#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <memory>

namespace bdata = boost::unit_test::data;

#include <interconnect.hpp>

BOOST_AUTO_TEST_CASE(test_interconnect_ctor) {
    {
        auto ict = Interconnects(32, InterconnectType::banyan);
        BOOST_TEST(ict.pin_interconnect->num_ports() == 32);
    }
    
    {
        auto ict = Interconnects(32, InterconnectType::banyan_exp_1);
        BOOST_TEST(ict.pin_interconnect->num_ports() == 64);
    }
    
    {
        auto ict = Interconnects(32, InterconnectType::banyan_exp_2);
        BOOST_TEST(ict.pin_interconnect->num_ports() == 128);
    }
    
    {
        auto ict = Interconnects(32, InterconnectType::banyan_exp_3);
        BOOST_TEST(ict.pin_interconnect->num_ports() == 256);
    }
}
