
#include "layer.hpp"

using namespace std;

Layer::Layer(string layer_name, 
                tile_dim_map* x_tile_dims, 
                tile_dim_map* w_tile_dims, 
                tuple<int, int, int> no_tiles, 
                tuple<int, int> input_size, 
                tuple<int, int> weight_size,
                bool is_conv,
                tuple<int, int> conv_kernel_size,
                list<string>* dependencies){
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

    this->is_conv = is_conv;
    this->conv_kernel_size = conv_kernel_size;
    
    this->x_tiles = new map<tuple<int, int>, X_Tile*>();
    this->w_tiles = new map<tuple<int, int>, W_Tile*>();
    this->p_tiles = new map<tuple<int, int, int>, P_Tile*>();
}

Layer Layer::create_copy(string suffix){
    string layer_name = this->layer_name + suffix;

    list<string>* dependencies = new list<string>();
    for(auto it_dep = this->dependencies->begin(); it_dep != this->dependencies->end(); it_dep++){
        string dep_name = *it_dep + suffix;
        dependencies->push_back(dep_name);
    }

    Layer new_layer(layer_name, this->x_tile_dims, this->w_tile_dims, this->no_tiles, this->input_size, this->weight_size, this->is_conv, this->conv_kernel_size, dependencies);

    for(auto tile_it = this->x_tiles->begin(); tile_it != this->x_tiles->end(); tile_it++){
        tuple<int, int> ind = tile_it->first;
        X_Tile* x_tile_orig = tile_it->second;

        X_Tile* x_tile_new = new X_Tile(new_layer.layer_name, x_tile_orig->id, x_tile_orig->dims, x_tile_orig->precision);
        (*new_layer.x_tiles)[ind] = x_tile_new;                
    }

    for(auto tile_it = this->w_tiles->begin(); tile_it != this->w_tiles->end(); tile_it++){
        tuple<int, int> ind = tile_it->first;
        W_Tile* w_tile_orig = tile_it->second;

        W_Tile* w_tile_new = new W_Tile(new_layer.layer_name, w_tile_orig->id, w_tile_orig->dims, w_tile_orig->precision);
        (*new_layer.w_tiles)[ind] = w_tile_new;                
    }

    for(auto tile_it = this->p_tiles->begin(); tile_it != this->p_tiles->end(); tile_it++){
        tuple<int, int, int> ind = tile_it->first;
        P_Tile* p_tile_orig = tile_it->second;

        P_Tile* p_tile_new = new P_Tile(new_layer.layer_name, p_tile_orig->id, p_tile_orig->dims, p_tile_orig->precision);
        (*new_layer.p_tiles)[ind] = p_tile_new;                
    }

    for(auto op_it = this->main_ops.begin(); op_it != this->main_ops.end(); op_it++ ){
        tuple<int, int, int> op_ind = op_it->first;

        int i = get<0>(op_ind);
        int j = get<1>(op_ind);
        int k = get<2>(op_ind);

        X_Tile* x_tile = (*new_layer.x_tiles)[make_tuple(i,j)];
        W_Tile* w_tile = (*new_layer.w_tiles)[make_tuple(j,k)];
        P_Tile* pout_tile = (*new_layer.p_tiles)[make_tuple(i,j,k)];

        MultOp* op_new = new MultOp(new_layer.layer_name, op_ind, x_tile, w_tile, pout_tile);
        new_layer.main_ops[make_tuple(i,j,k)] = op_new;
    }

    for(auto list_it = this->post_ops.begin(); list_it != this->post_ops.end(); list_it++){
        list<AggrOp*> op_list_old = list_it->second;

        auto op_it = op_list_old.begin();
        while(op_it != op_list_old.end()){
            AggrOp* op_old = *op_it;
            tuple<int, int, int> op_id = op_old->op_ind;
            
            Op* op1_old = op_old->operand1;
            Op* op2_old = op_old->operand2;
            Op* op1_new;
            Op* op2_new;

            if (op1_old->is_multop){
                op1_new = new_layer.get_mainop_by_index(op1_old->op_ind);
            }
            else{
                op1_new = new_layer.get_postop_by_index(op1_old->op_ind, 0);
            }
            if (op2_old->is_multop){
                op2_new = new_layer.get_mainop_by_index(op2_old->op_ind);
            }
            else{
                op2_new = new_layer.get_postop_by_index(op2_old->op_ind, 1);
            }

            P_Tile* pout_tile = new P_Tile(new_layer.layer_name, op_id, op_old->pout_tile->dims);

            AggrOp* aggr_op1 = new AggrOp(new_layer.layer_name, op_id, op1_new, op2_new, pout_tile, 0);
            AggrOp* aggr_op2 = new AggrOp(new_layer.layer_name, op_id, op1_new, op2_new, pout_tile, 1);

            new_layer.post_ops[list_it->first].push_back(aggr_op1);
            new_layer.post_ops[list_it->first].push_back(aggr_op2);

            aggr_op1->set_pair(aggr_op2);

            //Skip two ops because they are pairs.
            op_it++;
            op_it++;
        }
    }

    return new_layer;
}

Layer::~Layer(){}

Layers::Layers(){
    this->layer_list = new list<Layer>();
}

Layers::~Layers(){
    delete this->layer_list;
}

void Layers::import_layers(json j){
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

        list<string>* dependencies = new list<string>();
        for(auto it_dep = deps.begin(); it_dep != deps.end(); it_dep++){
            string dep_name = it_dep->get<string>(); // remove 
            dependencies->push_back(dep_name);
        }

        bool is_conv = false;
        tuple<int,int> conv_kernel_size (-1,-1);
        string layer_type =  j["layers"][layer_name]["layer_type"].get<string>();

        if (layer_type == "Conv2D"){
            is_conv = true;
            conv_kernel_size = make_tuple(gemm_op["kernel_size"][0].get<int>(), gemm_op["kernel_size"][1].get<int>());
        }
        Layer layer(layer_name, x_tile_dim, w_tile_dim, no_tiles, input_size, weight_size, is_conv, conv_kernel_size, dependencies);
        this->layer_list->push_back(layer);
    }
}

void Layer::create_main_ops(){
    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            X_Tile* x_tile = new X_Tile(this->layer_name, make_tuple(i,j), (*this->x_tile_dims)[make_tuple(i, j)], 1);
            (*this->x_tiles)[make_tuple(i,j)] = x_tile;
        }
    }

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int k = 0; k < get<2>(this->no_tiles); k++){
            W_Tile* w_tile = new W_Tile(this->layer_name, make_tuple(j,k), (*this->w_tile_dims)[make_tuple(j, k)], 1);
            (*this->w_tiles)[make_tuple(j,k)] = w_tile;
        }
    }

    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int i = 0; i < get<0>(this->no_tiles); i++){
            for (int k = 0; k < get<2>(this->no_tiles); k++){
                P_Tile* pout_tile = new P_Tile(this->layer_name, 
                                                make_tuple(i,j,k), 
                                                make_tuple(get<0>((*this->x_tile_dims)[make_tuple(i, j)]), 
                                                            get<1>((*this->w_tile_dims)[make_tuple(j, k)])), 2);
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

            int j = 0;
            while (unconsumed_ops.size()/2 >= 1){
                Op* op1 = unconsumed_ops.front();
                unconsumed_ops.pop_front();

                Op* op2 = unconsumed_ops.front();
                unconsumed_ops.pop_front();

                tuple<int, int, int> op_id = make_tuple(i, j, k);
                P_Tile* pout_tile = new P_Tile(layer_name, op_id, op1->pout_tile->dims);

                AggrOp* aggr_op1 = new AggrOp(this->layer_name, op_id, op1, op2, pout_tile, 0);
                unconsumed_ops.push_back(aggr_op1);
                post_op_list->push_back(aggr_op1);

                AggrOp* aggr_op2 = new AggrOp(this->layer_name, op_id, op1, op2, pout_tile, 1);
                post_op_list->push_back(aggr_op2);

                aggr_op1->set_pair(aggr_op2);
                j++;
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
        }
    }

    bank_cnt = 0;
    for (int j = 0; j < get<1>(this->no_tiles); j++){
        for (int k = 0; k < get<2>(this->no_tiles); k++){
            (*this->w_tiles)[make_tuple(j,k)]->assign_bank(banks->get_bank_by_id(bank_cnt, data_type::W));
            bank_cnt = (bank_cnt+1) % banks->no_banks;
        }
    }
}


MultOp* Layer::get_mainop_by_index(tuple<int, int, int> index){
    return this->main_ops[index];
}

AggrOp* Layer::get_postop_by_index(tuple<int, int, int> index, bool flip){
    int i = get<0>(index);
    int k = get<2>(index);

    list<AggrOp*> op_list = this->post_ops[make_tuple(i,k)];

    for (auto it = op_list.begin(); it != op_list.end(); it++){
        if ( (*it)->op_ind == index && (*it)->flip == flip ){
            return *it;
        }
    }

    return nullptr;
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

ostream& operator<<(ostream& os, const Layers& layers)
{
    for(auto it = layers.layer_list->begin(); it != layers.layer_list->end(); it++){
        os << it->layer_name << endl;
    }
    return os;
}