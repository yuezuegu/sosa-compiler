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

using namespace std;

namespace po = boost::program_options;

int main(int ac, char* av[]){
    logger_setup();

    int no_array, no_rows, no_cols;
    int bank_size;
    float bandwidth;
    InterconnectType interconnect_type;
    string work_dir;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show options")
        ("no_rows,r", po::value<int>(&no_rows)->default_value(32), "number of rows in a systolic array")
        ("no_cols,c", po::value<int>(&no_cols)->default_value(32), "number of columns in a systolic array")
        ("no_array,N", po::value<int>(&no_array)->default_value(8), "number of systolic arrays")
        ("memory_bw,M", po::value<float>(&bandwidth)->default_value(100), "memory bandwidth in GB/s")
        ("bank_size,S", po::value<int>(&bank_size)->default_value(1<<19), "SRAM bank size")
        ("ict_type,I", po::value<InterconnectType>(&interconnect_type)->default_value(InterconnectType::crossbar), "interconnect type (see enum members)")
        ("work_dir,d", po::value<string>(&work_dir)->default_value("experiments/tmp"), "directory for input/output files")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    std::cout << (unsigned) interconnect_type << "\n";

    std::cout << "Running with: " <<
        "no_array = " << no_array << " " <<
        "no_rows = " << no_rows << " " <<
        "no_cols = " << no_cols << " " <<
        "interconnect_type = " << interconnect_type <<
        "\n";

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

    Layers* layers = new Layers();
    layers->import_layers(jin);

    Arrays* arrays = new Arrays(no_array, no_rows, no_cols);
    PostProcessors* post_processors = new PostProcessors(no_array);
    Banks* banks = new Banks(no_array, bank_size);

    Interconnects* interconnects = new Interconnects(no_array, interconnect_type);
    cout << "interconnects->x_interconnect->name() = " << interconnects->x_interconnect->name() << endl;

    float freq = 1e9;
    bandwidth = bandwidth * ((1 << 30) / freq);

    Dram* dram = new Dram(bandwidth);

    Compiler* compiler = new Compiler(arrays, banks, interconnects, post_processors, dram);

    #ifdef COMPILER_MULTITHREADING
    // TODO enable passing the number of workers as a command-line argument argument
    compiler->enable_multithreading(48);
    #endif

    cout << layers;
    compiler->compile(layers);

    int no_repeat = jin["no_repeat"].get<int>();
    compiler->duplicate_schedule(layers, no_repeat);

    compiler->run_cycle_model();

    cout << "Finished succesfully" << endl;
    cout << "# of main rounds: " << compiler->no_main_rounds() << endl;
    cout << "# of post rounds: " << compiler->no_post_rounds() << endl;

    cout << "Total no. of cycles: " << compiler->no_cycles << endl;

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
    jout["no_cycles"] = compiler->no_cycles;
    jout["no_main_rounds"] = compiler->no_main_rounds();
    jout["no_post_rounds"] = compiler->no_post_rounds();
    jout["interconnect_tdp"] = interconnects->tdp(no_cols);
    jout["no_ops"] = jin["no_ops"].get<long>();

    cout << jout.dump() << endl;

    output_file << jout.dump();
    output_file.close();

    delete layers;
    delete arrays;
    delete post_processors;
    delete banks;
    delete interconnects;
    delete compiler;

    return 0;
}