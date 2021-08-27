#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>
#include <iostream>

#include "layer.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"

using namespace std;

class Compiler{
    public:
        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;
        PostProcessors* post_processors;

        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors);
        void compile(Layers* layers);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);
        void post_op_placement(int r, AggrOp* op);

    private:
};

#endif /* COMPILER_HPP */
