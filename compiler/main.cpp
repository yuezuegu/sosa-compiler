#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <tuple>
#include <string>

#include "nlohmann/json.hpp"
#include "layer.hpp"

using namespace std;

using json = nlohmann::json;

int main(){
    string fname = "/home/yuezuegu/sosa-compiler/experiments/run-2021_07_08-16_03_48/90ac8b8d4f96b11014dd755a32c4f5d5/precompiled_model.json";

    json j;
    ifstream input_file;
    input_file.open(fname, ifstream::in);
    if(!input_file.is_open()){
        cout << "Input file " << fname << " cannot be opened." << endl;
        exit(1);
    }

    input_file >> j;
    input_file.close();

    for (json::iterator it = j.begin(); it != j.end(); ++it) {
        string layer_name = it.key();
        cout << "layer name: " << layer_name; 
        if (it.value().is_null()){
            cout << ": " << it.value() << endl;
        }
        else{
            cout << endl;

            map<tuple<int, int>, tuple<int, int>> x_tile_dim;
            auto val = it.value()["x_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    x_tile_dim[{stoi(key_ind1), stoi(key_ind2)}] = {val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>()};
                }
            }

            map<tuple<int, int>, tuple<int, int>> w_tile_dim;
            val = it.value()["w_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    w_tile_dim[{stoi(key_ind1), stoi(key_ind2)}] = {val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>()};
                }
            }

            tuple<int, int, int> no_tiles = {it.value()["no_tiles"][0].get<int>(), it.value()["no_tiles"][1].get<int>(), it.value()["no_tiles"][2].get<int>()};
            tuple<int, int> input_size = {it.value()["input_size"][0].get<int>(), it.value()["input_size"][1].get<int>()};
            tuple<int, int> weight_size = {it.value()["weight_size"][0].get<int>(), it.value()["weight_size"][1].get<int>()};

            Layer layer(layer_name, x_tile_dim, w_tile_dim, no_tiles, input_size, weight_size);
            // cout << "no_tiles: " << it.value()["no_tiles"] << endl;
            // cout << "input_size: " << it.value()["input_size"] << endl;
            // cout << "weight_size: " << it.value()["weight_size"] << endl;

        }
        
    }

    cout << "Finished succesfully" << endl;
    return 0;
}