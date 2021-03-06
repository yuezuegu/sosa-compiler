#include <iostream>
#include <string>

#include <pybind11/pybind11.h>

#include "compiler.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"
#include "logger_setup.hpp"
#include "dram.hpp"

using namespace std;

string csim(string json_dump, int no_array, int no_rows, int no_cols, int bank_size, float bandwidth, int prefetch_limit, string ict_type) {
    logger_setup(boost::log::trivial::severity_level::error);

    InterconnectType interconnect_type;
    std::istringstream iss{ict_type};
    iss >> interconnect_type;
    if (!iss) {
        return "";
    }

    Arrays* arrays = new Arrays(no_array, no_rows, no_cols);
    PostProcessors* post_processors = new PostProcessors(no_array);
    Banks* banks = new Banks(no_array, bank_size);

    Interconnects* interconnects = new Interconnects(no_array, interconnect_type);

    float freq = 1e9;
    bandwidth = bandwidth * ((1 << 30) / freq);

    Dram* dram = new Dram(bandwidth, prefetch_limit);

    Compiler* compiler = new Compiler(arrays, banks, interconnects, post_processors, dram);

    json jin = json::parse(json_dump);

    Model* model = new Model();
    for (json::iterator it = jin.begin(); it != jin.end(); ++it) {
        string key = it.key();
        if (!key.compare("args")) continue;

        string model_name (key);

        json j (jin[model_name]);

        model = new Model(model_name, j);

        cout << model;
        compiler->compile(model);
        
        compiler->duplicate_schedule(model, model->no_repeat);
    }

    compiler->run_cycle_model();

    json jout(jin["args"]);

    jout["no_array"] = no_array;
    jout["interconnect_type"] = interconnect_type; //TODO: Replace with string value
    jout["no_cycles"] = compiler->no_cycles;
    jout["no_main_rounds"] = compiler->no_main_rounds();
    jout["no_post_rounds"] = compiler->no_post_rounds();
    jout["interconnect_tdp"] = interconnects->tdp(no_cols);

    jout["x_tiles_bw_usage"] = dram->x_tiles_bw_usage;
    jout["w_tiles_bw_usage"] = dram->w_tiles_bw_usage;
    jout["p_tiles_bw_usage"] = dram->p_tiles_bw_usage;
    jout["total_bw_usage"] = dram->x_tiles_bw_usage + dram->w_tiles_bw_usage + dram->p_tiles_bw_usage;
    jout["no_post_ops"] = post_processors->total_no_ops(); //INT operation (elementwise)
    jout["no_ops"] = arrays->total_no_ops(); //INT operation (elementwise)
    jout["total_sram_read_bytes"] = arrays->total_sram_read_bytes() + post_processors->total_sram_read_bytes(); //Bytes
    jout["total_sram_write_bytes"] = arrays->total_sram_write_bytes() + post_processors->total_sram_write_bytes(); //Bytes
    jout["memory_stall_cycles"] = compiler->memory_stall_cycles;
    jout["no_layers"] = model->layer_list->size();
    jout["total_no_gemm_ops"] = model->total_no_gemm_ops();

    delete arrays;
    delete post_processors;
    delete banks;
    delete interconnects;
    delete compiler;

    return jout.dump();
}

PYBIND11_MODULE(pythonbinder, m) {
    m.doc() = "pybind11 plugin";
    m.def("csim", &csim, "C-simulator for multi-pod systolic arrays");
}