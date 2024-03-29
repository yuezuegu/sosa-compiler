#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <tuple>
#include <string>
#include <list>

#include "compiler.hpp"
#include "array.hpp"
#include "interconnect.hpp"
#include "post_processor.hpp"
#include "logger_setup.hpp"
#include "dram.hpp"

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

namespace po = boost::program_options;

int main(int ac, char* av[]){
    int no_array, no_rows, no_cols;
    int bank_size;
    float bandwidth;
    int prefetch_limit;
    InterconnectType interconnect_type;
    boost::log::trivial::severity_level log_level;
    string work_dir;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show options")
        ("no_rows,r", po::value<int>(&no_rows)->default_value(32), "number of rows in a systolic array")
        ("no_cols,c", po::value<int>(&no_cols)->default_value(32), "number of columns in a systolic array")
        ("no_array,N", po::value<int>(&no_array)->default_value(128), "number of systolic arrays")
        ("memory_bw,M", po::value<float>(&bandwidth)->default_value(1200), "memory bandwidth in GB/s")
        ("prefetch,P", po::value<int>(&prefetch_limit)->default_value(100), "No of rounds allowed for prefetching")
        ("bank_size,S", po::value<int>(&bank_size)->default_value(524288), "SRAM bank size")
        ("ict_type,I", po::value<InterconnectType>(&interconnect_type)->default_value(InterconnectType::banyan_exp_1), "interconnect type (see enum members)")
        //Possible options for ict_type: crossbar, benes_copy, benes_vanilla, banyan_exp_0, banyan_exp_1, banyan_exp_2, banyan_exp_3, banyan_exp_4
        ("work_dir,d", po::value<string>(&work_dir)->default_value("experiments/tmp"), "directory for input/output files")
        ("log_level,l", po::value<boost::log::trivial::severity_level>(&log_level)->default_value(boost::log::trivial::severity_level::error), "log level");
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    logger_setup(log_level);

    std::cout << (unsigned) interconnect_type << "\n";

    std::cout << "Running with: " <<
        "no_array = " << no_array << " " <<
        "no_rows = " << no_rows << " " <<
        "no_cols = " << no_cols << " " <<
        "memory bandwidth = " << bandwidth << " " <<
        "bank size = " << bank_size << " " <<
        "prefetch = " << prefetch_limit << " " <<
        "interconnect_type = " << interconnect_type << " " <<
        "\n";

    // Load the precompiled model in the JSON format. Precompiler must be invoked first to generate this file.
    json jin;
    ifstream input_file;
    string ifname = work_dir + "/precompiled_model.json";
    input_file.open(ifname, ifstream::in);
    if(!input_file.is_open()){
        cout << "Input file " << ifname << " cannot be opened." << endl;
        exit(1);
    }
    input_file >> jin;
    input_file.close();

    Arrays* arrays = new Arrays(no_array, no_rows, no_cols);
    PostProcessors* post_processors = new PostProcessors(no_array);
    Banks* banks = new Banks(no_array, bank_size);
    Interconnects* interconnects = new Interconnects(no_array, interconnect_type);

    //TODO: Make this parametric
    float freq = 1e9;

    //Convert GB/s to Bytes per cycle
    bandwidth = bandwidth * ((1 << 30) / freq);

    Dram* dram = new Dram(bandwidth, prefetch_limit);

    Compiler* compiler = new Compiler(arrays, banks, interconnects, post_processors, dram);

    #ifdef COMPILER_MULTITHREADING
    // TODO enable passing the number of workers as a command-line argument argument
    compiler->enable_multithreading(48);
    #endif

    // Run the compilation for each DNN model provided
    Model* model = new Model();
    for (json::iterator it = jin.begin(); it != jin.end(); ++it) {
        string key = it.key();
        if (!key.compare("args")) continue;

        string model_name (key);
        json j (jin[model_name]);

        model = new Model(model_name, j);

        compiler->compile(model);
        
        compiler->duplicate_schedule(model, model->no_repeat);
    }
    
    cout << "Finished succesfully" << endl;
    cout << "# of main rounds: " << compiler->no_main_rounds() << endl;
    cout << "# of post rounds: " << compiler->no_post_rounds() << endl;

    compiler->run_cycle_model();
    cout << "Total no. of cycles: " << compiler->no_cycles << endl;

    // Save results in the JSON format
    ofstream output_file;
    string ofname = work_dir + "/sim_results.json";
    output_file.open(ofname, ofstream::out);
    if(!output_file.is_open()){
        cout << "Output file " << ofname << " cannot be opened." << endl;
        exit(1);
    }

    json jout(jin["args"]);
    jout["no_array"] = no_array;
    jout["interconnect_type"] = interconnect_type; //TODO: Replace with string value
    jout["bank_size"] = bank_size;
    jout["bandwidth"] = bandwidth;
    jout["no_cycles"] = compiler->no_cycles;
    jout["no_main_rounds"] = compiler->no_main_rounds();
    jout["no_post_rounds"] = compiler->no_post_rounds();
    jout["interconnect_tdp"] = interconnects->tdp(no_cols);
    //jout["no_ops"] = jin["no_ops"].get<long>();
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

    cout << "Computing interconn_total_mbytes..." << endl;
    jout["interconn_total_mbytes"] = compiler->interconn_total_mbytes();

    cout << "Computing interconn_total_mbytes_with_multicast..." << endl;
    jout["interconn_total_mbytes_with_multicast"] = compiler->interconn_total_mbytes_with_multicast();

    cout << "total_no_gemm_ops: " << model->total_no_gemm_ops() << endl;

    cout << jout.dump() << endl;

    output_file << jout.dump();
    output_file.close();

    delete arrays;
    delete post_processors;
    delete banks;
    delete interconnects;
    delete compiler;

    return 0;
}
