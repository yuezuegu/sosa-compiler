#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>
#include <iostream>

#include "layer.hpp"
#include "array.hpp"
#include "interconnect.hpp"

using namespace std;

class Compiler{
    public:
        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;

        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects);
        void compile(Layers* layers);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);

    private:
};

#endif
