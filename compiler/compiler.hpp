#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>
#include <iostream>
#include <algorithm>
#include <memory>

#include "layer.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"

#include <boost/log/trivial.hpp>

using namespace std;

class Compiler{
    public:
        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;
        PostProcessors* post_processors;

        int no_cycles;

        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors);
        void compile(Layers* layers);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);
        void post_op_placement(int r, AggrOp* op);
        int no_main_rounds();
        int no_post_rounds();
        void run_cycle_model();
        void duplicate_schedule(int no_repeat);
    private:
};

#endif /* COMPILER_HPP */
