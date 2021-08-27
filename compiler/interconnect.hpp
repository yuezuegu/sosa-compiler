#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <map>

#include "bank.hpp"
#include "array.hpp"
#include "post_processor.hpp"

using namespace std;

template<class T>
class Interconnect{
    public:
        int no_arrays;

        Interconnect(int no_arrays);
        void apply_permute(map<Bank*, T*> permute);
        bool is_route_free(Bank* bank, T* array);
    private:

};

class Interconnects{
    public:
        Interconnect<Array>* x_interconnect;
        Interconnect<Array>* w_interconnect;
        Interconnect<Array>* pin_interconnect;
        Interconnect<Array>* pout_interconnect;
        Interconnect<PostProcessor>* pp_in1_interconnect;
        Interconnect<PostProcessor>* pp_in2_interconnect;
        Interconnect<PostProcessor>* pp_out_interconnect;

        Interconnects(int no_arrays);
    private:


};

#endif /* INTERCONNECT_HPP */
