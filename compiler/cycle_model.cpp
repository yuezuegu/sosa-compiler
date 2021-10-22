
#include <iostream>

#include "compiler.hpp"
#include "logger_setup.hpp"

#include <boost/log/trivial.hpp>

using namespace std;

int main(int ac, char* av[]){
    boost::log::trivial::severity_level log_level = boost::log::trivial::info;
    logger_setup(log_level);

    //TODO: Does not produce the same results because op->retire never free any tiles. 
    //That is because serialization of tile->input_of does not work correctly so setting op->retired = True does not change the op in input_of.
    //Fix this first before using this code.


    Compiler* compiler;
    {
        // create and open an archive for input
        std::ifstream ifs("../experiments/tmp/schedule1.dat");
        if (ifs.fail()){
            cout << "Input file could not be opened." << endl;
        }

        boost::archive::text_iarchive ia(ifs);
        // read class state from archive
        ia >> compiler;
        // archive and stream closed when destructors are called
    }

    cout << compiler->sram_round_trip << endl;
    cout << compiler->pp_latency_offset << endl;

    compiler->run_cycle_model();

    cout << "no_cycles: " << compiler->no_cycles << endl;

    return 0;
}