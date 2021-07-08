#include <iostream>
#include <fstream>

#include "nlohmann/json.hpp"

using namespace std;

using json = nlohmann::json;


int main(){
    string fname = "experiments/run-2021_07_08-16_03_48/90ac8b8d4f96b11014dd755a32c4f5d5/precompiled_model.json";
    cout << "Parsing json file: " << fname << endl;

    json j;
    ifstream input_file(fname, ifstream::in);
    input_file >> j;


    return 0;
}