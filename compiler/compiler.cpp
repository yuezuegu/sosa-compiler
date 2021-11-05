
#include "compiler.hpp"

using namespace std;

#ifdef COMPILER_MULTITHREADING

struct Compiler::WorkerData {
    // interconnects should not be shared among different threads
    // have your own copy.
    Interconnects interconnects;

    list<Bank *> *avail_x_banks;
    list<Bank *> *avail_w_banks;
    list<Bank *> *avail_pout_banks;

    Op *in_op1;
    Op *in_op2;
};

struct Compiler::PlacementClosure {
    // is this a placement operation for the postpp?
    bool pp = false;

    // job-specific data
    list<Array *>::iterator sa_it;
    list<Bank *>::iterator x_bank_it;
    list<Bank *>::iterator w_bank_it;
    list<Bank *>::iterator p_bank_it;

    list<PostProcessor *>::iterator pp_it;
    list<Bank *>::iterator pout_it;

    bool operator()(std::size_t idx, const WorkerData &wd) {
        if (pp) {
            // closure for postprocessor op placement
            if (!wd.interconnects.pp_in1_interconnect->is_route_free(wd.in_op1->pout_tile->bank, *pp_it)){
                return false;
            }
            if (!wd.interconnects.pp_in2_interconnect->is_route_free(wd.in_op2->pout_tile->bank, *pp_it)){
                return false;
            }

            for (auto pout_it = wd.avail_pout_banks->begin(); pout_it != wd.avail_pout_banks->end(); pout_it++){
                if (!wd.interconnects.pp_out_interconnect->is_route_free(*pout_it, *pp_it)){
                    continue;
                }

                this->pout_it = pout_it;
                
                return true;
            }

            return false ;
        }
        else {
            // closure for op placement
            for(auto x_bank_it = wd.avail_x_banks->begin(); x_bank_it != wd.avail_x_banks->end(); x_bank_it++){
                if (!wd.interconnects.x_interconnect->is_route_free(*x_bank_it, *sa_it)){
                    continue;
                }

                for(auto w_bank_it = wd.avail_w_banks->begin(); w_bank_it != wd.avail_w_banks->end(); w_bank_it++){
                    if (!wd.interconnects.w_interconnect->is_route_free(*w_bank_it, *sa_it)){
                        continue;
                    }

                    for(auto p_bank_it = wd.avail_pout_banks->begin(); p_bank_it != wd.avail_pout_banks->end(); p_bank_it++){
                        if (!wd.interconnects.pout_interconnect->is_route_free(*p_bank_it, *sa_it)){
                            continue;
                        }

                        x_bank_it = x_bank_it;
                        w_bank_it = w_bank_it;
                        p_bank_it = p_bank_it;

                        return true;
                    }
                }
            }

            return false;
        }
    }
};

#endif

Compiler::Compiler(Arrays* arrays, Banks* banks, Interconnects* interconnects, PostProcessors* post_processors, Dram* dram){
    this->arrays = arrays;
    this->banks = banks;
    this->interconnects = interconnects;
    this->post_processors = post_processors;
    this->dram = dram;

    this->sram_round_trip = this->interconnects->x_interconnect->data_req_latency() + this->interconnects->x_interconnect->data_read_latency();
    this->pp_latency_offset = this->interconnects->pout_interconnect->data_write_latency();
}

void Compiler::compile(Layers* layers){
    while (!layers->all_layers_scheduled()){
        for(auto it = layers->begin(); it != layers->end(); it++){
            if (it->is_scheduled) continue;

            bool all_deps_scheduled = true;
            int init_round = 0;
            for(auto it_dep = it->dependencies->begin(); it_dep != it->dependencies->end(); it_dep++){
                Layer* layer = layers->get_layer_by_name((*it_dep));
                if (!layer->is_scheduled){
                    all_deps_scheduled = false;
                    break;
                }

                if (layer->end_round >= init_round){
                    init_round = layer->end_round+1;
                }
            }

            if (!all_deps_scheduled) continue;

            compile_layer(&(*it), init_round);
            cout << it->layer_name << ": start round: " << it->start_round << " end round: " << it->end_round << endl;
        }
    }
}

void Compiler::compile_layer(Layer* layer, int init_round){
    layer->create_main_ops();
    layer->init_banks(this->banks);

    layer->start_round = -1;
    layer->end_round = -1;

    for (int j = 0; j < get<1>(layer->no_tiles); j++){
        for (int i = 0; i < get<0>(layer->no_tiles); i++){
            for (int k = 0; k < get<2>(layer->no_tiles); k++){
                MultOp* op = layer->get_mainop_by_index(make_tuple(i,j,k));

                int r = init_round;
                while (true){
                    this->op_placement(r, op);

                    if (op->is_placed()){
                        if (layer->start_round == -1){
                            layer->start_round = r;
                        }
                        else{
                            if (r < layer->start_round){
                                layer->start_round = r;
                            }
                        }
                        if (r > layer->end_round) layer->end_round = r;
                        break;
                    }
                    else{
                        r++;
                    }
                    
                }
            }
        }
    }

    layer->create_post_ops(this->arrays, this->interconnects);

    for (int i = 0; i < get<0>(layer->no_tiles); i++){
        for (int k = 0; k < get<2>(layer->no_tiles); k++){
            
            list<AggrOp*>* post_op_list = &layer->post_ops[make_tuple(i,k)];

            for (auto it = post_op_list->begin(); it != post_op_list->end(); it++){
                AggrOp* post_op = *it;

                int r = post_op->max_round()+1;
                while(true){
                    this->post_op_placement(r, post_op);

                    if (post_op->is_placed()){
                        if (r < layer->start_round){
                            throw runtime_error("Something is wrong, r cannot be smaller than start round");
                        }
                        if (r > layer->end_round) layer->end_round = r;
                        break;
                    }
                    else{
                        r++;
                    }
                }
            }

            if (post_op_list->empty()){
                int max_round = -1;
                MultOp* final_op = nullptr;
                for (int j = 0; j < get<1>(layer->no_tiles); j++){
                    MultOp* mult_op = layer->get_mainop_by_index(make_tuple(i,j,k));
                    if (mult_op->round_placed > max_round){
                        final_op = mult_op;
                        max_round = mult_op->round_placed;
                    }
                }
                final_op->is_finalop = true;
            }
            else{
                int max_round = -1;
                AggrOp* final_op = nullptr;
                for (auto it = post_op_list->begin(); it != post_op_list->end(); it++){
                    if ((*it)->round_placed > max_round){
                        final_op = *it;
                        max_round = (*it)->round_placed;
                    }
                }
                final_op->is_finalop = true;
                final_op->pair_op->is_finalop = true;
            }
        }
    }

    if (layer->start_round == -1) layer->start_round = init_round;
    if (layer->end_round == -1) layer->end_round = init_round - 1;
    layer->is_scheduled = true;
}

void Compiler::post_op_placement(int r, AggrOp* op){
    Op* in_op1 = op->get_op1();
    Op* in_op2 = op->get_op2();

    unique_ptr< list<PostProcessor*> > avail_pps (this->post_processors->available_pps(r));
    if (avail_pps->empty()){
        return;
    }

    unique_ptr< map<PostProcessor*, Bank*> > pin1_permute (this->post_processors->get_pin1_permute(r));
    if(this->post_processors->check_pin1_bank_conflict(r, in_op1->pout_tile)){
        return;
    }

    unique_ptr< map<PostProcessor*, Bank*> > pin2_permute (this->post_processors->get_pin2_permute(r));
    if(this->post_processors->check_pin2_bank_conflict(r, in_op2->pout_tile)){
        return;
    }

    unique_ptr< map<PostProcessor*, Bank*> > pout_permute (this->post_processors->get_pout_permute(r));
    
    unique_ptr< list<Bank*> > avail_pout_banks;
    if (op->pout_tile->bank != nullptr){
        if(this->post_processors->check_pout_bank_conflict(r, op->pout_tile)){
            return;
        }
        avail_pout_banks = unique_ptr< list<Bank*> >(new list<Bank*>());
        avail_pout_banks->push_back(op->pout_tile->bank);
    }
    else{
        avail_pout_banks = unique_ptr< list<Bank*> >(new list<Bank*>(*this->banks->get_p_banks()));
        for (auto it = pout_permute->begin(); it != pout_permute->end(); it++){
            avail_pout_banks->remove(it->second);
        }
    }
    if(avail_pout_banks->empty()) return;

    this->interconnects->pp_in1_interconnect->apply_permute(pin1_permute.get());
    this->interconnects->pp_in2_interconnect->apply_permute(pin2_permute.get());
    this->interconnects->pp_out_interconnect->apply_permute(pout_permute.get());

#ifdef COMPILER_MULTITHREADING
    if (pls_){
        for (std::size_t i = 0; i < pls_->num_workers(); ++i) {
            auto &wd = pls_->worker_data(i);
            wd.interconnects.copy_from(this->interconnects);
            wd.avail_pout_banks = avail_pout_banks.get();
            wd.in_op1 = in_op1;
            wd.in_op2 = in_op2;
        }

        pls_->begin();

        for (auto pp_it = avail_pps->begin(); pp_it != avail_pps->end() && (!pls_ || pls_->should_continue()); pp_it++){
            pls_->append_job(PlacementClosure{
                true,
                {}, // sa_it
                {}, // x_bank_it
                {}, // w_bank_it
                {}, // p_bank_it
                pp_it, // pp_it
                {} // pout_it
            });
        }
        pls_->end();
        auto result = pls_->result();

        if (result) {
            op->pout_tile->assign_bank(*result->closure.pout_it);
            (*result->closure.pp_it)->assign_op(r, op);
            
            BOOST_LOG_TRIVIAL(info) <<
                "Post-op placed: layer_name: " << op->layer_name <<
                "\tround: " << r << "\tsa: " << (*result->closure.pp_it)->id <<
                "\tpout_bank: " << (*result->closure.pout_it)->id;
            return ;
        }

        return ;
    }
#endif

    for (auto pp_it = avail_pps->begin(); pp_it != avail_pps->end(); pp_it++){
        if (!this->interconnects->pp_in1_interconnect->is_route_free(in_op1->pout_tile->bank, *pp_it)){
            continue;
        }
        if (!this->interconnects->pp_in2_interconnect->is_route_free(in_op2->pout_tile->bank, *pp_it)){
            continue;
        }

        for (auto pout_it = avail_pout_banks->begin(); pout_it != avail_pout_banks->end(); pout_it++){
            if (!this->interconnects->pp_out_interconnect->is_route_free(*pout_it, *pp_it)){
                continue;
            }

            op->pout_tile->assign_bank(*pout_it);
            (*pp_it)->assign_op(r, op);
            
            BOOST_LOG_TRIVIAL(info) <<
                "Post-op placed: layer_name: " << op->layer_name <<
                "\tround: " << r << "\tsa: " << (*pp_it)->id << "\tpout_bank: " << (*pout_it)->id;

            return;
        }
    }
}

void Compiler::op_placement(int r, MultOp* op){
    unique_ptr< list<Array*> > avail_arrays (this->arrays->available_arrays(r));

    if (avail_arrays->empty()){
        return;
    }

    unique_ptr< map<Array*, Bank*> > pout_permute (this->arrays->get_pout_permute(r));

    unique_ptr< list<Bank*> > avail_pout_banks;
    if (op->pout_tile->bank != nullptr){
        if (this->arrays->check_pout_bank_conflict(r, op->pout_tile)){
            return;
        }

        avail_pout_banks = unique_ptr< list<Bank*> >(new list<Bank*>());
        avail_pout_banks->push_back(op->pout_tile->bank);
    }
    else{
        avail_pout_banks = unique_ptr< list<Bank*> >(new list<Bank*>(*this->banks->get_p_banks()));
        for (auto it = pout_permute->begin(); it != pout_permute->end(); it++){
            avail_pout_banks->remove(it->second);
        }
    }
    if(avail_pout_banks->empty()) return;
    
    unique_ptr< map<Array*, Bank*> > x_permute (this->arrays->get_x_permute(r));
    BOOST_LOG_TRIVIAL(info) << "op_placement: x_permute = " << [&] {
        std::vector<int> permute(banks->no_banks, -1);
        for (auto it = x_permute->begin(); it != x_permute->end(); it++) {
            permute[it->first->id] = it->second->id;
        }
        std::ostringstream oss;
        for (auto &x: permute) {
            oss << x << ", ";
        }
        return oss.str();
    }();

    unique_ptr< list<Bank*> > avail_x_banks;
    if (op->x_tile->bank != nullptr){
        if (this->arrays->check_x_bank_conflict(r, op->x_tile)){
            return;
        }
        
        avail_x_banks = unique_ptr< list<Bank*> >(new list<Bank*>());
        avail_x_banks->push_back(op->x_tile->bank);
    }
    else{
        avail_x_banks = unique_ptr< list<Bank*> >(new list<Bank*>(*this->banks->get_x_banks()));
        for (auto it = x_permute->begin(); it != x_permute->end(); it++){
            avail_x_banks->remove(it->second);
        }
    }
    if(avail_x_banks->empty()) return;
    
    unique_ptr< map<Array*, Bank*> > w_permute (this->arrays->get_w_permute(r));
    unique_ptr< list<Bank*> > avail_w_banks;
    if (op->w_tile->bank != nullptr){
        if (this->arrays->check_w_bank_conflict(r, op->w_tile)){
            return;
        }

        avail_w_banks = unique_ptr< list<Bank*> >(new list<Bank*>());
        avail_w_banks->push_back(op->w_tile->bank);
    }
    else{
        avail_w_banks = unique_ptr< list<Bank*> >(new list<Bank*>(*this->banks->get_w_banks()));
        for (auto it = w_permute->begin(); it != w_permute->end(); it++){
            avail_w_banks->remove(it->second);
        }
    }
    if(avail_w_banks->empty()) return;

    this->interconnects->pout_interconnect->apply_permute(pout_permute.get());
    this->interconnects->x_interconnect->apply_permute(x_permute.get());    
    this->interconnects->w_interconnect->apply_permute(w_permute.get());    

    //random_shuffle(avail_arrays.begin(), avail_arrays.end(), *this->random_generator);
    
#ifdef COMPILER_MULTITHREADING
    if (pls_) {
        for (std::size_t i = 0; i < pls_->num_workers(); ++i) {
            auto &wd = pls_->worker_data(i);
            wd.interconnects.copy_from(this->interconnects);
            wd.avail_x_banks = avail_x_banks.get();
            wd.avail_w_banks = avail_w_banks.get();
            wd.avail_pout_banks = avail_pout_banks.get();
        }

        pls_->begin();

        for(auto sa_it = avail_arrays->begin(); sa_it != avail_arrays->end() && (!pls_ || pls_->should_continue()); sa_it++){
            pls_->append_job(PlacementClosure{
                true,
                sa_it, // sa_it
                {}, // x_bank_it
                {}, // w_bank_it
                {}, // p_bank_it
                {}, // pp_it
                {} // pout_it
            });
        }

        pls_->end();
        auto result = pls_->result();

        if (result) {
            op->x_tile->assign_bank(*result->closure.x_bank_it);
            op->w_tile->assign_bank(*result->closure.w_bank_it);
            op->pout_tile->assign_bank(*result->closure.p_bank_it);
            (*result->closure.sa_it)->assign_op(r, op);

            BOOST_LOG_TRIVIAL(info) <<
                "Op placed: layer_name: " << op->layer_name <<
                "\tind: " <<  get<0>(op->op_ind) <<
                "-" << get<1>(op->op_ind) <<
                "-" << get<2>(op->op_ind) <<
                "\tround: " << r <<
                "\tsa: " << (*result->closure.sa_it)->id <<
                "\tx bank id: " << (op->x_tile->bank->id);

            return ;
        }
    }

#endif

    for(auto sa_it = avail_arrays->begin(); sa_it != avail_arrays->end(); sa_it++){
        for(auto x_bank_it = avail_x_banks->begin(); x_bank_it != avail_x_banks->end(); x_bank_it++){
            if (!this->interconnects->x_interconnect->is_route_free(*x_bank_it, *sa_it)){
                continue;
            }

            for(auto w_bank_it = avail_w_banks->begin(); w_bank_it != avail_w_banks->end(); w_bank_it++){
                if (!this->interconnects->w_interconnect->is_route_free(*w_bank_it, *sa_it)){
                    continue;
                }

                for(auto p_bank_it = avail_pout_banks->begin(); p_bank_it != avail_pout_banks->end(); p_bank_it++){
                    if (!this->interconnects->pout_interconnect->is_route_free(*p_bank_it, *sa_it)){
                        continue;
                    }

                    op->x_tile->assign_bank(*x_bank_it);
                    op->w_tile->assign_bank(*w_bank_it);
                    op->pout_tile->assign_bank(*p_bank_it);
                    (*sa_it)->assign_op(r, op);

                    BOOST_LOG_TRIVIAL(info) << "Op placed: layer_name: " << op->layer_name << "\tind: " <<  get<0>(op->op_ind) << "-" << get<1>(op->op_ind) << "-" << get<2>(op->op_ind) << "\tround: " << r << "\tsa: " << (*sa_it)->id << "\tx bank id: " << (op->x_tile->bank->id);

                    return;
                }
            }
        }
    }
}



int Compiler::no_main_rounds(){
    int max_rounds = 0;
    for(auto it = this->arrays->array_map->begin(); it != this->arrays->array_map->end(); it++ ){
        if (it->second->last_no_round > max_rounds){
            max_rounds = it->second->last_no_round;
        }
    }
    return max_rounds;
}

int Compiler::no_post_rounds(){
    int max_rounds = 0;
    for(auto it = this->post_processors->pp_map->begin(); it != this->post_processors->pp_map->end(); it++ ){
        if (it->second->last_no_round > max_rounds){
            max_rounds = it->second->last_no_round;
        }
    }
    return max_rounds;
}


void Compiler::duplicate_schedule(Layers* layers, int no_repeat){
    int no_layers = layers->layer_list->size();

    int no_main_rounds = this->no_main_rounds();
    int no_post_rounds = this->no_post_rounds();
    int max_no_rounds = no_main_rounds > no_post_rounds ? no_main_rounds : no_post_rounds;

    for (int i = 1; i < no_repeat; i++){

        auto layer_it = layers->begin();
        for(int l = 0; l < no_layers; l++){
            string suffix =  "_copy" + to_string(i);
            
            Layer new_layer =  layer_it->create_copy(suffix);

            for (auto op_it = layer_it->main_ops.begin(); op_it != layer_it->main_ops.end(); op_it++){
                tuple<int, int, int> op_ind = op_it->first;
                MultOp* op_old = op_it->second;
                MultOp* op_new = new_layer.main_ops[op_ind];

                op_new->x_tile->assign_bank(op_old->x_tile->bank);
                op_new->w_tile->assign_bank(op_old->w_tile->bank);
                op_new->pout_tile->assign_bank(op_old->pout_tile->bank);

                //TODO: This fails when N=1
                int old_round = op_old->round_placed;
                int new_round = old_round + i*(max_no_rounds+1) + 1;
                op_old->array_placed->assign_op(new_round, op_new);

                if (op_old->pin_op != nullptr){
                    op_new->assign_pin(new_layer.main_ops[op_old->pin_op->op_ind]);
                }
            }

            for (auto list_it = layer_it->post_ops.begin(); list_it != layer_it->post_ops.end(); list_it++){
                list<AggrOp*> op_list = list_it->second;

                auto op_it = op_list.begin();
                while(op_it != op_list.end()){
                    AggrOp* post_op_old = *op_it;
                    AggrOp* post_op_new = new_layer.get_postop_by_index(post_op_old->op_ind, post_op_old->flip);

                    int old_round = post_op_old->round_placed;
                    int new_round = old_round + i*max_no_rounds + 1;

                    post_op_new->pout_tile->assign_bank(post_op_old->pout_tile->bank);
                    post_op_old->pp_placed->assign_op(new_round, post_op_new);

                    op_it++;
                }
            }

            layers->layer_list->push_back(new_layer);
            layer_it++;
        }
    }
}


int calc_max_required_memory(Arrays* arrays, PostProcessors* post_processors, int r){
    list<W_Tile*>* w_tiles = arrays->get_w_tiles(r+1);
    list<X_Tile*>* x_tiles = arrays->get_x_tiles(r);
    list<P_Tile*>* pout_tiles = arrays->get_pout_tiles(r);
    list<P_Tile*>* pin_tiles = arrays->get_pin_tiles(r);
    list<P_Tile*>* pp_pin1_tiles = post_processors->get_pin1_tiles(r);
    list<P_Tile*>* pp_pin2_tiles = post_processors->get_pin2_tiles(r);
    list<P_Tile*>* pp_pout_tiles = post_processors->get_pout_tiles(r);

    int max_mem_size;
    max_mem_size = accumulate(w_tiles->begin(), w_tiles->end(), 0.0, [](const int a, const W_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(x_tiles->begin(), x_tiles->end(), 0.0, [](const int a, const X_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(pout_tiles->begin(), pout_tiles->end(), 0.0, [](const int a, const P_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(pin_tiles->begin(), pin_tiles->end(), 0.0, [](const int a, const P_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(pp_pin1_tiles->begin(), pp_pin1_tiles->end(), 0.0, [](const int a, const P_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(pp_pin2_tiles->begin(), pp_pin2_tiles->end(), 0.0, [](const int a, const P_Tile* b){return a + b->memory_size;});
    max_mem_size += accumulate(pp_pout_tiles->begin(), pp_pout_tiles->end(), 0.0, [](const int a, const P_Tile* b){return a + b->memory_size;});

    delete w_tiles;
    delete x_tiles;
    delete pout_tiles;
    delete pin_tiles;
    delete pp_pin1_tiles;
    delete pp_pin2_tiles;
    delete pp_pout_tiles;

    return max_mem_size;
}

void Compiler::check_if_livelock(list<P_Tile*>* p_tiles){
    P_Tile* p_tile = *find_if(p_tiles->begin(), p_tiles->end(), [](P_Tile* p_tile){return !p_tile->is_allocated();});
    if(p_tile->bank->spawn_queue->front().second == p_tile){
        this->livelock_detected = true;
    }
}

bool Compiler::check_if_data_ready(Arrays* arrays, PostProcessors* post_processors, int r){
    bool is_ready;

    list<W_Tile*>* w_tiles = arrays->get_w_tiles(r+1);
    is_ready = all_of(w_tiles->begin(), w_tiles->end(), [](W_Tile* w_tile){ return w_tile->is_allocated(); });
    if (!is_ready) return false;
    delete w_tiles;

    list<X_Tile*>* x_tiles = arrays->get_x_tiles(r);
    is_ready = all_of(x_tiles->begin(), x_tiles->end(), [](X_Tile* x_tile){ return x_tile->is_allocated(); });
    if (!is_ready) return false;
    delete x_tiles;

    list<P_Tile*>* pout_tiles = arrays->get_pout_tiles(r);
    is_ready = all_of(pout_tiles->begin(), pout_tiles->end(), [](P_Tile* p_tile){ return p_tile->is_allocated(); });
    if (!is_ready){
        this->check_if_livelock(pout_tiles);
        return false;
    }
    delete pout_tiles;
    
    list<P_Tile*>* pin_tiles = arrays->get_pin_tiles(r);
    is_ready = all_of(pin_tiles->begin(), pin_tiles->end(), [](P_Tile* p_tile){ return p_tile->is_allocated(); });
    if (!is_ready) return false;
    delete pin_tiles;

    list<P_Tile*>* pp_pin1_tiles = post_processors->get_pin1_tiles(r);
    is_ready = all_of(pp_pin1_tiles->begin(), pp_pin1_tiles->end(), [](P_Tile* p_tile){ return p_tile->is_allocated(); });
    if (!is_ready) return false;
    delete pp_pin1_tiles;
    
    list<P_Tile*>* pp_pin2_tiles = post_processors->get_pin2_tiles(r);
    is_ready = all_of(pp_pin2_tiles->begin(), pp_pin2_tiles->end(), [](P_Tile* p_tile){ return p_tile->is_allocated(); });
    if (!is_ready) return false;
    delete pp_pin2_tiles;
    
    list<P_Tile*>* pp_pout_tiles = post_processors->get_pout_tiles(r);
    is_ready = all_of(pp_pout_tiles->begin(), pp_pout_tiles->end(), [](P_Tile* p_tile){ return p_tile->is_allocated(); });
    if (!is_ready){
        this->check_if_livelock(pp_pout_tiles);
        return false;
    }
    delete pp_pout_tiles;
    
    return true;
}



void Compiler::create_memory_fifo(){
    int no_rounds = this->no_main_rounds();
    if (this->no_post_rounds() > no_rounds) no_rounds = this->no_post_rounds();

    int r = 0;
    while(r <= no_rounds){
        list<MultOp*>* sch = this->arrays->get_schedule(r);

        for (auto it = sch->begin(); it != sch->end(); it++){
            if (*it == nullptr) continue;
            (*it)->pout_tile->bank->spawn_queue->push_back(make_pair(r,(*it)->pout_tile));
        }

        for (auto it = sch->begin(); it != sch->end(); it++){
            if (*it == nullptr) continue;
            this->dram->load_queue->push_back(make_pair(r,(*it)->w_tile));
        }

        for (auto it = sch->begin(); it != sch->end(); it++){
            if (*it == nullptr) continue;
            this->dram->load_queue->push_back(make_pair(r,(*it)->x_tile));
        }

        for (auto it = sch->begin(); it != sch->end(); it++){
            if (*it == nullptr) continue;
            if ((*it)->pin_op == nullptr) continue;
            this->dram->load_queue->push_back(make_pair(r,(*it)->pin_op->pout_tile));
        }

        list<AggrOp*>* postops = this->post_processors->get_schedule(r);
        for (auto it = postops->begin(); it != postops->end(); it++){
            if (*it == nullptr) continue;
            (*it)->pout_tile->bank->spawn_queue->push_back(make_pair(r,(*it)->pout_tile));
            this->dram->load_queue->push_back(make_pair(r,(*it)->pin1_tile));
            this->dram->load_queue->push_back(make_pair(r,(*it)->pin2_tile));
        }

        delete sch;
        delete postops;
        r++;
    }
}

void Compiler::run_cycle_model(){
    this->create_memory_fifo();

    int main_rounds = this->no_main_rounds();
    int post_rounds = this->no_post_rounds();
    int max_rounds = main_rounds > post_rounds ? main_rounds : post_rounds;

    int r = 0;
    int arr_cycle = 0;
    int pp_cycle = 0;

    // Warm-up
    while (!this->arrays->is_weights_buffered(r)){
        list<W_Tile*>* w_tiles = this->arrays->get_w_tiles(r);
        if( all_of(w_tiles->begin(), w_tiles->end(), [](W_Tile* w_tile){ return w_tile->is_allocated(); }) ){
            this->arrays->init_weight_buffering(r);
        }
        delete w_tiles;

        this->arrays->update();
        this->dram->update(this->banks->get_p_banks(), r);
        arr_cycle++;
    }

    this->banks->spawn(r);

    while(!check_if_data_ready(this->arrays, this->post_processors, r)){
        this->dram->update(this->banks->get_p_banks(), r);
        arr_cycle++;
    }

    pp_cycle = arr_cycle + this->pp_latency_offset;

    int memory_stall = 0;
    int max_mem_size;

    int round_clk = 0;
    bool new_round = true;
    while(r < max_rounds){
        if (new_round){
            // Round start
            round_clk = 0;

            this->arrays->init_tile_op(r);
            this->arrays->init_weight_buffering(r+1);
            this->post_processors->init_tile_op(r);

            new_round = false;

            // if ( r % 10 == 0){
            //     cout << "Round: " << r << " completed with at array clock cycle: " << arr_cycle << " pp clock cycle: " << pp_cycle << endl;
            // }
        }

        //Propagate clock
        this->arrays->update();
        this->post_processors->update();
        this->dram->update(this->banks->get_p_banks(), r);

        if (this->arrays->is_tile_op_done(r) && 
            this->arrays->is_weights_buffered(r+1) && 
            this->post_processors->is_tile_op_done(r) && 
            round_clk >= this->sram_round_trip){

            //this->dram->store(r+1);
            this->banks->spawn(r+1);
            
            if(check_if_data_ready(this->arrays, this->post_processors, r+1)){

                this->banks->garbage_collect(r);

                r++;
                new_round = true;
                memory_stall = 0;
                //Round end
                BOOST_LOG_TRIVIAL(info) << "Main round " << r << " takes " << round_clk << "clock cycles";
            }
            else{
                memory_stall++;

                BOOST_LOG_TRIVIAL(info) << "Memory stalling: " << memory_stall << " at r: " << r << " clk: " << arr_cycle;

                max_mem_size = calc_max_required_memory(this->arrays, this->post_processors, r+1);
                float memory_timeout = 100 * (float)max_mem_size / this->dram->bandwidth;

                // if (this->livelock_detected) {
                //     cout << "Memory stalling: " << memory_stall << endl << "Livelock detected. Increase your sram size" << endl;
                //     this->no_cycles = -1;
                //     return;
                // }

                if(memory_stall > memory_timeout){    
                    cout << "Execution timed out. Probably not enough sram" << endl;
                    this->no_cycles = -1;
                    return;
                }
            }
        }

        arr_cycle++;
        pp_cycle++;
        round_clk++;
    }

    this->no_cycles = arr_cycle > pp_cycle ? arr_cycle : pp_cycle;
}

#ifdef COMPILER_MULTITHREADING

void Compiler::enable_multithreading(std::size_t num_workers) {
    pls_ = std::make_unique<multithreading::ParallelLinearSearch<PlacementClosure, WorkerData>>(num_workers);
    for (std::size_t i = 0; i < num_workers; ++i) {
        pls_->worker_data(i).interconnects.construct(interconnects->N, interconnects->type);
    }
}

void Compiler::disable_multithreading() {
    pls_ = nullptr;
}

#endif

Compiler::~Compiler() {
    #ifdef COMPILER_MULTITHREADING
    for (std::size_t i = 0; i < pls_->num_workers(); ++i) {
        delete std::any_cast<Interconnects *>(pls_->worker_data(i));
    }
    #endif
}
