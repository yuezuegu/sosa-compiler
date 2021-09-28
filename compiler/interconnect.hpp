#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>
#include <ios>
#include <string>
#include "interconnect/interconnect.hpp"

using namespace std;

enum class InterconnectType : unsigned {
    crossbar = 1 << 4, // TODO not yet implemented
    benes_copy = 2 << 4,
    benes_vanilla = 3 << 4,
    banyan = 4 << 4, // Vanilla Banyan network
    // expansion factor is given in log2
    banyan_exp_0 = banyan | 0, // 1 Banyan network
    banyan_exp_1 = banyan | 1, // 2 Banyan networks
    banyan_exp_2 = banyan | 2, // 4 Banyan networks
    banyan_exp_3 = banyan | 3, // 8 Banyan networks
    banyan_exp_4 = banyan | 4  // 16 Banyan networks
};

std::istream &operator>>(std::istream &in, InterconnectType &interconnect_type);
std::ostream &operator<<(std::ostream &out, InterconnectType interconnect_type);

InterconnectBase *generate_interconnect(InterconnectType type);

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
