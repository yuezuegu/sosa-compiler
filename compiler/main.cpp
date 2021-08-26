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

using namespace std;

int main(){
    string fname = "/home/yuezuegu/sosa-compiler/experiments/run-2021_07_08-16_03_48/90ac8b8d4f96b11014dd755a32c4f5d5/precompiled_model.json";

    int no_array = 4;
    int no_rows = 8;
    int no_cols = 8;
    int bank_size = 1 >> 20; //1 MB

    Layers* layers = new Layers(fname);
    Arrays* arrays = new Arrays(no_array, no_rows, no_cols);
    Banks* banks = new Banks(no_array, bank_size);
    Interconnects* interconnects = new Interconnects(no_array);

    Compiler* compiler = new Compiler(arrays, banks, interconnects);
    
    compiler->compile(layers);

    cout << "Finished succesfully" << endl;
    return 0;
}