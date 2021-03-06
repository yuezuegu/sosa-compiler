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
        tile_dim_map* x_tile_dims;
        tile_dim_map* w_tile_dims;
        tuple<int, int, int> no_tiles;
        tuple<int, int> input_size;
        tuple<int, int> weight_size;
        int no_gemm_ops {0};

        bool raw_input;

        bool is_conv;
        tuple<int, int> conv_kernel_size;

        map<tuple<int, int>, X_Tile*>* x_tiles;
        map<tuple<int, int>, W_Tile*>* w_tiles;
        map<tuple<int, int, int>, P_Tile*>* p_tiles;

        bool is_scheduled;
        int start_round;
        int end_round;
        list<string>* dependencies;

        Layer(string, tile_dim_map*, tile_dim_map*, tuple<int, int, int>, tuple<int, int>, tuple<int, int>, bool, bool, tuple<int, int>, list<string>*);
        ~Layer();
        
        void create_main_ops();
        void create_post_ops(Arrays* arrays, Interconnects* interconnects);
        void init_banks(Banks* banks);
        map<tuple<int, int, int>, MultOp*> main_ops;
        map<tuple<int, int>, list<AggrOp*>> post_ops;

        MultOp* get_mainop_by_index(tuple<int, int, int> index);
        AggrOp* get_postop_by_index(tuple<int, int, int> index, bool flip);
        
        Layer create_copy(string);

    private:
        
};


class Model {
    public:
        string model_name;
        int no_repeat;
        
        list<Layer>::iterator begin() noexcept { return layer_list->begin(); }
        list<Layer>::iterator end() { return layer_list->end(); }

        Model(){};
        Model(string model_name, json j){
            this->model_name = model_name;
            this->layer_list = new list<Layer>();

            this->import_layers(j);

        };
        ~Model(){delete this->layer_list;};

        void import_layers(json j);
        bool all_layers_scheduled();
        Layer* get_layer_by_name(string layer_name);
        friend ostream& operator<<(ostream& os, const Model& model);
        int total_no_gemm_ops();


        list<Layer>* layer_list;


};

#endif /* LAYER_HPP */
