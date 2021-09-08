#ifndef LAYER_HPP
#define LAYER_HPP


#include <iostream>
#include <fstream>
#include <map>
#include <tuple>
#include <list>
#include <iterator>

#include "ops.hpp"
#include "bank.hpp"
#include "array.hpp"
#include "interconnect.hpp"

#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

typedef map<tuple<int, int>, tuple<int, int>> tile_dim_map;

class Layer {
    public:
        string layer_name;
        tile_dim_map x_tile_dims;
        tile_dim_map w_tile_dims;
        tuple<int, int, int> no_tiles;
        tuple<int, int> input_size;
        tuple<int, int> weight_size;
        bool is_scheduled;
        list<Layer*> dependencies;

        Layer(string, tile_dim_map, tile_dim_map, tuple<int, int, int>, tuple<int, int>, tuple<int, int>, list<Layer*>);
        void create_main_ops();
        void create_post_ops(Arrays* arrays, Interconnects* interconnects);
        void init_banks(Banks* banks);
        map<tuple<int, int, int>, MultOp*> main_ops;
        map<tuple<int, int>, list<AggrOp*>> post_ops;

        MultOp* get_mainop_by_index(tuple<int, int, int> index);

        int start_round();
        int end_round();
        
    private:
        
};

class Layers{
    public:
        list<Layer>::iterator begin() noexcept { return layer_list.begin(); }
        list<Layer>::iterator end() { return layer_list.end(); }


        Layers(string fname);
        void import_layers(string fname);
        bool all_layers_scheduled();
        Layer* get_layer_by_name(string layer_name);
    private:
        list<Layer> layer_list;
};

#endif /* LAYER_HPP */
