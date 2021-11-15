#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>
#include <iostream>
#include <algorithm>
#include <memory>
#include <queue>
#include <fstream>

#include "layer.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"
#include "dram.hpp"

#include "parallel_linear_search.hpp"
#include <memory>

#include <boost/log/trivial.hpp>

#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

class Compiler{
    public:

        Arrays* arrays;
        Banks* banks;
        Interconnects* interconnects;
        PostProcessors* post_processors;
        Dram* dram;

        int no_cycles;
        int sram_round_trip;
        int pp_latency_offset;
        bool livelock_detected {false};
        int memory_stall_cycles;


        Compiler(){};
        Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors, Dram* dram);
        void compile(Model* model);
        void compile_layer(Layer* layer, int init_round);
        void op_placement(int r, MultOp* op);
        void post_op_placement(int r, AggrOp* op);
        int no_main_rounds();
        int no_post_rounds();
        void create_memory_fifo();
        void run_cycle_model();
        bool is_all_data_ready(Arrays* arrays, PostProcessors* post_processors, int r);
        void check_if_livelock(list<P_Tile*>* p_tiles);

        void duplicate_schedule(Model* model, int no_repeat);

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->no_cycles;
            ar & this->arrays;
            ar & this->banks;
            ar & this->post_processors;
            //ar & this->interconnects;
            ar & this->dram;
            ar & this->sram_round_trip;
            ar & this->pp_latency_offset;
        }

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
