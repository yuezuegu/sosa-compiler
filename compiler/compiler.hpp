#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>
#include <iostream>
#include <algorithm>
#include <memory>
#include <queue>

#include "layer.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"
#include "dram.hpp"

#include <boost/log/trivial.hpp>

using namespace std;

class Compiler{
    public:
        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;
        PostProcessors* post_processors;
        Dram* dram;

        int no_cycles;

        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors, Dram* dram);
        void compile(Layers* layers);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);
        void post_op_placement(int r, AggrOp* op);
        int no_main_rounds();
        int no_post_rounds();
        void create_memory_fifo();
        void run_cycle_model();
        void duplicate_schedule(Layers* layers, int no_repeat);
    private:
};

#endif /* COMPILER_HPP */
