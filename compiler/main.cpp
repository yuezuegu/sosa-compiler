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

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

using namespace std;

namespace po = boost::program_options;

int main(int ac, char* av[]){
    logger_setup();

    int no_array, no_rows, no_cols;
    InterconnectType interconnect_type;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show options")
        ("no_rows,r", po::value<int>(&no_rows)->default_value(32), "number of rows in a systolic array")
        ("no_cols,c", po::value<int>(&no_cols)->default_value(32), "number of columns in a systolic array")
        ("no_array,N", po::value<int>(&no_array)->default_value(32), "number of systolic arrays")
        ("ict_type,I", po::value<InterconnectType>(&interconnect_type)->default_value(InterconnectType::crossbar), "interconnect type (see enum members)")
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

    string fname = "experiments/tmp/precompiled_model.json";

    Layers* layers = new Layers();
    layers->import_layers(fname);

    Arrays* arrays = new Arrays(no_array, no_rows, no_cols);
    PostProcessors* post_processors = new PostProcessors(no_array);
    Banks* banks = new Banks(no_array);

    Interconnects* interconnects = new Interconnects(no_array, interconnect_type);

    Compiler* compiler = new Compiler(arrays, banks, interconnects, post_processors);

    compiler->compile(layers);

    cout << "Finished succesfully" << endl;
    cout << "# of main rounds: " << compiler->no_main_rounds() << endl;
    cout << "# of post rounds: " << compiler->no_post_rounds() << endl;

    // cout << "Total no. of cycles: " << compiler->get_cycles() << endl;

    delete layers;
    delete arrays;
    delete post_processors;
    delete banks;
    delete interconnects;
    delete compiler;

    return 0;
}