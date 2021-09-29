
#include "layer.hpp"

using namespace std;

Layer::Layer(string layer_name, 
                tile_dim_map* x_tile_dims, 
                tile_dim_map* w_tile_dims, 
                tuple<int, int, int> no_tiles, 
                tuple<int, int> input_size, 
                tuple<int, int> weight_size,
                list<Layer*>* dependencies){
    this->layer_name = layer_name;
    this->x_tile_dims = x_tile_dims;
    this->w_tile_dims = w_tile_dims;
    this->no_tiles = no_tiles;
    this->input_size = input_size;
    this->weight_size = weight_size;
    this->is_scheduled = false;
    this->start_round = -1;
    this->end_round = -1;
    this->dependencies = dependencies;
    this->x_tiles = new map<tuple<int, int>, X_Tile*>();
    this->w_tiles = new map<tuple<int, int>, W_Tile*>();
    this->p_tiles = new map<tuple<int, int, int>, P_Tile*>();
}

Layer::~Layer(){
    // delete this->x_tiles;
    // delete this->w_tiles;
    // delete this->p_tiles;
}

Layers::Layers(){
    this->layer_list = new list<Layer>();
}

Layers::~Layers(){
    delete this->layer_list;
}

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

    for (json::iterator it = j["order"].begin(); it != j["order"].end(); ++it) {
        string layer_name = it.value();
        auto deps = j["layers"][layer_name]["deps"];
        auto gemm_op = j["layers"][layer_name]["gemm_op"];

        tile_dim_map* x_tile_dim = new tile_dim_map();
        tile_dim_map* w_tile_dim = new tile_dim_map();
        tuple<int, int, int> no_tiles = make_tuple(0,0,0);
        tuple<int, int> input_size = make_tuple(0,0);
        tuple<int, int> weight_size = make_tuple(0,0);

        if (!gemm_op.is_null()){
            auto val = gemm_op["x_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    (*x_tile_dim)[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            val = gemm_op["w_tile_dim"];
            for (json::iterator it2 = val.begin(); it2 != val.end(); it2++){
                auto key_ind1 = it2.key();
                for (json::iterator it3 = val[key_ind1].begin(); it3 != val[key_ind1].end(); it3++){
                    auto key_ind2 = it3.key();
                    (*w_tile_dim)[make_tuple(stoi(key_ind1), stoi(key_ind2))] = make_tuple(val[key_ind1][key_ind2][0].get<int>(), val[key_ind1][key_ind2][1].get<int>());
                }
            }

            no_tiles = make_tuple(gemm_op["no_tiles"][0].get<int>(), gemm_op["no_tiles"][1].get<int>(), gemm_op["no_tiles"][2].get<int>());
            input_size = make_tuple(gemm_op["input_size"][0].get<int>(), gemm_op["input_size"][1].get<int>());
            weight_size = make_tuple(gemm_op["weight_size"][0].get<int>(), gemm_op["weight_size"][1].get<int>());
        }

        list<Layer*>* dependencies = new list<Layer*>();
        for(auto it_dep = deps.begin(); it_dep != deps.end(); it_dep++){
            string dep_name = it_dep->get<string>(); // remove 
            Layer* dep_layer = this->get_layer_by_name(dep_name);
            if (dep_layer != nullptr){
                dependencies->push_back(dep_layer);
            }   
        }

        cout << layer_name << endl;
        Layer layer(layer_name, x_tile_dim, w_tile_dim, no_tiles, input_size, weight_size, dependencies);
        this->layer_list->push_back(layer);
    }
}

void Layer::create_main_ops(){
    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            X_Tile* x_tile = new X_Tile(this->layer_name, make_tuple(i,j), (*this->x_tile_dims)[make_tuple(i, j)]);
            (*this->x_tiles)[make_tuple(i,j)] = x_tile;
        }
    }

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int k = 0; k < get<2>(this->no_tiles); k++){
            W_Tile* w_tile = new W_Tile(this->layer_name, make_tuple(j,k), (*this->w_tile_dims)[make_tuple(j, k)]);
            (*this->w_tiles)[make_tuple(j,k)] = w_tile;
        }
    }

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            for (int k = 0; k < get<2>(this->no_tiles); k++){
                P_Tile* pout_tile = new P_Tile(this->layer_name, 
                                                make_tuple(i,j,k), 
                                                make_tuple(get<0>((*this->x_tile_dims)[make_tuple(i, j)]), 
                                                            get<1>((*this->w_tile_dims)[make_tuple(j, k)])));
                (*this->p_tiles)[make_tuple(i,j,k)] = pout_tile;
            }
        }
    }

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            for (int k = 0; k < get<2>(this->no_tiles); k++){
                tuple<int, int, int> op_ind = make_tuple(i,j,k);
                X_Tile* x_tile = (*this->x_tiles)[make_tuple(i,j)];
                W_Tile* w_tile = (*this->w_tiles)[make_tuple(j,k)];
                P_Tile* pout_tile = (*this->p_tiles)[make_tuple(i,j,k)];

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

                P_Tile* pout_tile = new P_Tile(layer_name, make_tuple(-1, -1, -1), op1->pout_tile->dims);

                AggrOp* aggr_op1 = new AggrOp(this->layer_name, op1, op2, pout_tile, 0);
                unconsumed_ops.push_back(aggr_op1);
                post_op_list->push_back(aggr_op1);

                AggrOp* aggr_op2 = new AggrOp(this->layer_name, op1, op2, pout_tile, 1);
                post_op_list->push_back(aggr_op2);

                aggr_op1->set_pair(aggr_op2);
            }
        }
    }



}

void Layer::init_banks(Banks* banks){
    int bank_cnt = 0;

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            (*this->x_tiles)[make_tuple(i,j)]->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::X));
            bank_cnt = (bank_cnt+1) % banks->no_banks;

            // for (int k = 0; k < get<2>(this->no_tiles); k++){
            //     MultOp* op = this->get_mainop_by_index(make_tuple(i,j,k));
            //     op->x_tile->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::X));
            //     bank_cnt = (bank_cnt+1) % banks->no_banks;
            // }
        }
    }

    bank_cnt = 0;
    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int k = 0; k < get<2>(this->no_tiles); k++){
            (*this->w_tiles)[make_tuple(j,k)]->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::W));
            bank_cnt = (bank_cnt+1) % banks->no_banks;
            // MultOp* op = this->get_mainop_by_index(make_tuple(i,j,k));
            // op->w_tile->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::W));
            // bank_cnt = (bank_cnt+1) % banks->no_banks;
        }
    }
}


MultOp* Layer::get_mainop_by_index(tuple<int, int, int> index){
    return this->main_ops[index];
}


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
        if (it->layer_name.compare(layer_name) == 0){
            return &(*it);
        }
    }

    return nullptr;
}