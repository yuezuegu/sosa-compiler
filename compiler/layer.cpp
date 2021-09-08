
#include "layer.hpp"

using namespace std;

Layer::Layer(string layer_name, 
                tile_dim_map x_tile_dims, 
                tile_dim_map w_tile_dims, 
                tuple<int, int, int> no_tiles, 
                tuple<int, int> input_size, 
                tuple<int, int> weight_size,
                list<Layer*> dependencies){
    this->layer_name = layer_name;
    this->x_tile_dims = x_tile_dims;
    this->w_tile_dims = w_tile_dims;
    this->no_tiles = no_tiles;
    this->input_size = input_size;
    this->weight_size = weight_size;
    this->is_scheduled = false;
    this->dependencies = dependencies;
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
        auto deps = it.value()["deps"];
        auto gemm_op = it.value()["gemm_op"];

        map<tuple<int, int>, tuple<int, int>> x_tile_dim;
        map<tuple<int, int>, tuple<int, int>> w_tile_dim;
        tuple<int, int, int> no_tiles = make_tuple(0,0,0);
        tuple<int, int> input_size = make_tuple(0,0);
        tuple<int, int> weight_size = make_tuple(0,0);

        if (!gemm_op.is_null()){
            auto val = gemm_op["x_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    x_tile_dim[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            val = gemm_op["w_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    w_tile_dim[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            no_tiles = make_tuple(gemm_op["no_tiles"][0].get<int>(), gemm_op["no_tiles"][1].get<int>(), gemm_op["no_tiles"][2].get<int>());
            input_size = make_tuple(gemm_op["input_size"][0].get<int>(), gemm_op["input_size"][1].get<int>());
            weight_size = make_tuple(gemm_op["weight_size"][0].get<int>(), gemm_op["weight_size"][1].get<int>());

        }

        list<Layer*> dependencies;
        for(auto it = deps.begin(); it != deps.end(); it++){
            string dep_name = it->get<string>(); // remove 
            Layer* dep_layer = this->get_layer_by_name(dep_name);
            if (dep_layer != nullptr){
                dependencies.push_back(dep_layer);
            }   
        }

        Layer layer(layer_name, x_tile_dim, w_tile_dim, no_tiles, input_size, weight_size, dependencies);
        this->layer_list.push_back(layer);
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

void Layer::create_post_ops(Arrays* arrays, Interconnects* interconnects){

    for (int i = 0; i < get<0>(this->no_tiles); i++){
        for (int k = 0; k < get<2>(this->no_tiles); k++){
            
            list<Op*> unconsumed_ops;
            for (int j = 0; j < get<1>(this->no_tiles); j++){
                unconsumed_ops.push_back(this->get_mainop_by_index(make_tuple(i,j,k)));
            }

            for (int j1 = 0; j1 < get<1>(this->no_tiles); j1++){
                MultOp* op1 = this->get_mainop_by_index(make_tuple(i,j1,k));
                int r1 = op1->round_placed;

                for (int j2 = 0; j2 < get<1>(this->no_tiles); j2++){
                    MultOp* op2 = this->get_mainop_by_index(make_tuple(i,j2,k));

                    if (op2->pin_op != nullptr){
                        continue;
                    }

                    int r2 = op2->round_placed;
                    if (r2 <= r1){
                        continue;
                    }

                    if(arrays->check_pin_bank_conflict(r2, op1->pout_tile)){
                        continue;
                    }

                    interconnects->pin_interconnect->apply_permute(arrays->get_pin_permute(r2));
                    if(!interconnects->pin_interconnect->is_route_free(op1->pout_tile->bank, op2->array_placed)){
                        continue;
                    }

                    op2->assign_pin(op1);
                    unconsumed_ops.remove(op1);
                    break;
                }
            }

            list<AggrOp*>* post_op_list = &this->post_ops[make_tuple(i,k)];

            while (unconsumed_ops.size()/2 >= 1){
                Op* op1 = unconsumed_ops.front();
                unconsumed_ops.pop_front();

                Op* op2 = unconsumed_ops.front();
                unconsumed_ops.pop_front();

                AggrOp* aggr_op1 = new AggrOp(this->layer_name, op1, op2, 0);
                unconsumed_ops.push_back(aggr_op1);
                post_op_list->push_back(aggr_op1);

                AggrOp* aggr_op2 = new AggrOp(this->layer_name, op1, op2, 1);
                post_op_list->push_back(aggr_op2);
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


MultOp* Layer::get_mainop_by_index(tuple<int, int, int> index){
    return this->main_ops[index];
};


bool Layers::all_layers_scheduled(){
    for (auto it = this->begin(); it != this->end(); it++ ){
        if (!it->is_scheduled){
            return false;
        }
    }

    return true;
}


Layer* Layers::get_layer_by_name(string layer_name){
    for (auto it = this->begin(); it != this->end(); it++){
        if (it->layer_name.compare(layer_name) != 0){
            return &(*it);
        }
    }

    return nullptr;
}