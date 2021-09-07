
#include "layer.hpp"

using namespace std;

Layer::Layer(string layer_name, 
                tile_dim_map x_tile_dims, 
                tile_dim_map w_tile_dims, 
                tuple<int, int, int> no_tiles, 
                tuple<int, int> input_size, 
                tuple<int, int> weight_size){
    this->layer_name = layer_name;
    this->x_tile_dims = x_tile_dims;
    this->w_tile_dims = w_tile_dims;
    this->no_tiles = no_tiles;
    this->input_size = input_size;
    this->weight_size = weight_size;
};

Layers::Layers(string fname){
    this->import_layers(fname);
};

void Layers::import_layers(string fname){
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

        if (!it.value().is_null()){
            map<tuple<int, int>, tuple<int, int>> x_tile_dim;
            auto val = it.value()["x_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    x_tile_dim[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            map<tuple<int, int>, tuple<int, int>> w_tile_dim;
            val = it.value()["w_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    w_tile_dim[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            tuple<int, int, int> no_tiles = make_tuple(it.value()["no_tiles"][0].get<int>(), it.value()["no_tiles"][1].get<int>(), it.value()["no_tiles"][2].get<int>());
            tuple<int, int> input_size = make_tuple(it.value()["input_size"][0].get<int>(), it.value()["input_size"][1].get<int>());
            tuple<int, int> weight_size = make_tuple(it.value()["weight_size"][0].get<int>(), it.value()["weight_size"][1].get<int>());

            Layer layer(layer_name, x_tile_dim, w_tile_dim, no_tiles, input_size, weight_size);

            this->layer_list.push_back(layer);
        }
    }
}

void Layer::create_main_ops(){
    for (int i = 0; i < get<0>(this->no_tiles); i++){
        for (int j = 0; j < get<1>(this->no_tiles); j++){
            for (int k = 0; k < get<2>(this->no_tiles); k++){
                tuple<int, int, int> op_ind = make_tuple(i,j,k);
                X_Tile* x_tile = new X_Tile(this->layer_name, make_tuple(i,j), this->x_tile_dims[make_tuple(i, j)]);
                W_Tile* w_tile = new W_Tile(this->layer_name, make_tuple(j,k), this->w_tile_dims[make_tuple(j, k)]);
                P_Tile* pout_tile = new P_Tile(this->layer_name, 
                                                make_tuple(i,j,k), 
                                                make_tuple(get<0>(this->x_tile_dims[make_tuple(i, j)]), 
                                                            get<1>(this->w_tile_dims[make_tuple(j, k)])));

                MultOp* op = new MultOp(this->layer_name, op_ind, x_tile, w_tile, pout_tile);
                this->main_ops[make_tuple(i,j,k)] = op;
            }
        }
    }
}

void Layer::init_banks(Banks* banks){
    int bank_cnt = 0;
    for (auto it = this->main_ops.begin(); it != this->main_ops.end(); it++){
        it->second->x_tile->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::X));
        bank_cnt = (bank_cnt+1) % banks->no_banks;
    }

    bank_cnt = 0;
    for (auto it = this->main_ops.begin(); it != this->main_ops.end(); it++){
        it->second->w_tile->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::W));
        bank_cnt = (bank_cnt+1) % banks->no_banks;
    }
}


void Layers::create_main_ops(){
    for (auto it = this->begin(); it != this->end(); it++ ){
        cout << "Processing layer: " << it->layer_name << endl;
        cout << "\tno_tiles: " << get<0>(it->no_tiles) << "-" << get<1>(it->no_tiles) << "-" <<get<2>(it->no_tiles) << endl;
        cout << "\tinput_size: " << get<0>(it->input_size) << "-" << get<1>(it->input_size) << endl;
        cout << "\tweight_size: " << get<0>(it->weight_size) << "-" << get<1>(it->weight_size) << endl;

        it->create_main_ops();
    }
}

MultOp* Layer::get_mainop_by_index(tuple<int, int, int> index){
    return this->main_ops[index];
};
