#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>
#include "interconnect/interconnect.hpp"

#include "bank.hpp"
#include "array.hpp"
#include "post_processor.hpp"

using namespace std;

enum InterconnectType : unsigned {
    crossbar = 1 << 4,
    benes_copy = 2 << 4,
    benes_vanilla = 3 << 4,
    banyan_exp_1 = 4 << 4 | 1,
    banyan = banyan_exp_1,
    banyan_exp_2 = 4 << 4 | 2,
    banyan_exp_3 = 4 << 4 | 3,
    banyan_exp_4 = 4 << 4 | 4
};

class Interconnects{
    public:
        InterconnectBase * x_interconnect;
        InterconnectBase * w_interconnect;
        InterconnectBase * pin_interconnect;
        InterconnectBase * pout_interconnect;
        InterconnectBase * pp_in1_interconnect;
        InterconnectBase * pp_in2_interconnect;
        InterconnectBase * pp_out_interconnect;
        
        Interconnects(int N, InterconnectType interconnect_type);
    private:


};

#endif /* INTERCONNECT_HPP */
