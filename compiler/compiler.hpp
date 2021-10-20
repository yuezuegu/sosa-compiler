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

#include "parallel_linear_search.hpp"
#include <memory>

#include <boost/log/trivial.hpp>

using namespace std;

class Compiler{
    public:
        // do not use pointers
        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;
        PostProcessors* post_processors;
        Dram* dram;

        int no_cycles;

        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors, Dram* dram);
        // in-place construction
        // Compiler(int no_arrays, int no_rows, int no_cols, int bank_size, );
        void compile(Layers* layers);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);
        void post_op_placement(int r, AggrOp* op);
        int no_main_rounds();
        int no_post_rounds();
        void create_memory_fifo();
        void run_cycle_model();

        void duplicate_schedule(Layers* layers, int no_repeat);

        #ifdef COMPILER_MULTITHREADING

        void enable_multithreading(std::size_t num_workers);
        void disable_multithreading();

        #endif

        ~Compiler();
    private:
        #ifdef COMPILER_MULTITHREADING

        struct PlacementClosure;
        struct WorkerData;
        
        std::unique_ptr<multithreading::ParallelLinearSearch<PlacementClosure, WorkerData>> pls_;
        
        #endif
};

#endif /* COMPILER_HPP */
