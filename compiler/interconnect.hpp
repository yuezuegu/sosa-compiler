#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>

#include "bank.hpp"
#include "array.hpp"

using namespace std;

class Interconnect{
    public:
        int no_arrays;

        Interconnect(int no_arrays);
        void apply_permute(map<Bank*, Array*> permute);
        bool is_route_free(Bank* bank, Array* array);
    private:

};

class Interconnects{
    public:
        Interconnect* x_interconnect;
        Interconnect* w_interconnect;
        Interconnect* pin_interconnect;
        Interconnect* pout_interconnect;
        Interconnect* pp_in1_interconnect;
        Interconnect* pp_in2_interconnect;
        Interconnect* pp_out_interconnect;

        Interconnects(int no_arrays);
    private:


};

#endif /* INTERCONNECT_HPP */
