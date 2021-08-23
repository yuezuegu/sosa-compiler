#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <tuple>
#include <string>
#include <list>

#include "compiler.hpp"

using namespace std;

int main(){
    string fname = "/home/yuezuegu/sosa-compiler/experiments/run-2021_07_08-16_03_48/90ac8b8d4f96b11014dd755a32c4f5d5/precompiled_model.json";

    int no_array = 4;
    int bank_size = 1 >> 20; //1 MB

    Layers* layers = new Layers(fname);
    Banks* banks = new Banks(no_array, bank_size);
    Compiler* compiler = new Compiler(banks);
    
    compiler->compile(layers);

    cout << "Finished succesfully" << endl;
    return 0;
}