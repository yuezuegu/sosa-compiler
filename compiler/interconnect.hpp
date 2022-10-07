#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>
#include <ios>
#include <string>
#include "interconnect/interconnect.hpp"

using namespace std;

enum class InterconnectType : unsigned {
    crossbar = 1 << 4,
    benes_copy = 2 << 4,
    benes_vanilla = 4 << 4,
    banyan = 8 << 4, // Vanilla Banyan network
    // expansion factor is given in log2
    banyan_exp_0 = banyan | 0, // 1 Banyan network
    banyan_exp_1 = banyan | 1, // 2 Banyan networks
    banyan_exp_2 = banyan | 2, // 4 Banyan networks
    banyan_exp_3 = banyan | 3, // 8 Banyan networks
    banyan_exp_4 = banyan | 4,  // 16 Banyan networks,
    bus = 16 << 4
};

std::istream &operator>>(std::istream &in, InterconnectType &interconnect_type);
std::ostream &operator<<(std::ostream &out, InterconnectType interconnect_type);

InterconnectBase *generate_interconnect(UnsignedInt n, InterconnectType type);

class Interconnects{
    public:
        InterconnectBase * x_interconnect;
        InterconnectBase * w_interconnect;
        InterconnectBase * pin_interconnect;
        InterconnectBase * pout_interconnect;
        InterconnectBase * pp_in1_interconnect;
        InterconnectBase * pp_in2_interconnect;
        InterconnectBase * pp_out_interconnect;
        int N;
        InterconnectType type;
        
        Interconnects();
        Interconnects(Interconnects const &other);
        Interconnects(int N, InterconnectType interconnect_type);
        float tdp(int switch_width);

        void construct(int N, InterconnectType interconnect_type);
        void copy_from(Interconnects *other);
        Interconnects *clone() const;

        ~Interconnects();
    private:


};

#endif /* INTERCONNECT_HPP */
