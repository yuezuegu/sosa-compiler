#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>
#include <interconnect/interconnect.hpp>

#include "bank.hpp"
#include "array.hpp"
#include "post_processor.hpp"

using namespace std;

class Interconnects{
    public:
        InterconnectBase * x_interconnect;
        InterconnectBase * w_interconnect;
        InterconnectBase * pin_interconnect;
        InterconnectBase * pout_interconnect;
        InterconnectBase * pp_in1_interconnect;
        InterconnectBase * pp_in2_interconnect;
        InterconnectBase * pp_out_interconnect;
        
    private:


};

#endif /* INTERCONNECT_HPP */
