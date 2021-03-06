

#include "post_processor.hpp"
#include "dram.hpp"

PostProcessor::PostProcessor(int id){
    this->id = id;
    this->last_no_round = 0;
    this->state = PP_STATE::idle;
    this->exec_cnt = 0;
    this->pin1_tile = nullptr;
    this->pin2_tile = nullptr;
    this->pout_tile = nullptr;
    this->curr_op = nullptr;
    this->no_add_ops = 0;
    this->sram_read_bytes = 0;
    this->sram_write_bytes = 0;
}

void PostProcessor::update(){
    if (this->state == PP_STATE::processing){
        if (this->exec_cnt < get<0>(this->pout_tile->dims)){
            this->exec_cnt++;
        }
        else{
            this->state = PP_STATE::done;
            this->curr_op->retire();
        }
    }
}

void PostProcessor::init_tile_op(int r){
    if (this->get_op(r) == nullptr) return;

    if ( this->state == PP_STATE::processing){
        throw runtime_error("PP is already in processing state");
    }

    this->exec_cnt = 0;
    this->state = PP_STATE::processing;
    this->curr_op = this->get_op(r);
    this->pin1_tile = this->curr_op->pin1_tile;
    this->pin2_tile = this->curr_op->pin2_tile;
    this->pout_tile = this->curr_op->pout_tile;

    this->no_add_ops += get<0>(this->pout_tile->dims) * get<1>(this->pout_tile->dims);
    this->sram_read_bytes += this->pin1_tile->memory_size;
    this->sram_read_bytes += this->pin2_tile->memory_size;
    this->sram_write_bytes += this->pout_tile->memory_size;
}

bool PostProcessor::is_tile_op_done(int r){
    if (this->get_op(r) == nullptr) return true;

    return this->state == PP_STATE::done;
}

void PostProcessor::assign_op(int r, AggrOp* op){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        this->schedule[r] = op;
        op->assign_to_pp(r, this);
        if (r > this->last_no_round){
            this->last_no_round = r;
        }
        return;
    }
    
    if (sch->second == nullptr){
        this->schedule[r] = op;
        op->assign_to_pp(r, this);
        return;
    }

    throw runtime_error("Cannot assign op to post-processor");
}

bool PostProcessor::is_schedule_empty(int r){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        return true;
    }

    if (sch->second == nullptr){
        return true;
    }  

    return false;
}

AggrOp* PostProcessor::get_op(int r){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        return nullptr;
    }

    return sch->second;
}

PostProcessors::PostProcessors(int no_pps){
    this->no_pps = no_pps;
    this->pp_map = new map<int, PostProcessor*>();

    for(int i = 0; i < no_pps; i++){
        (*this->pp_map)[i] = new PostProcessor(i);
    }
}

PostProcessors::~PostProcessors(){
    for(int i = 0; i < no_pps; i++){
        delete (*this->pp_map)[i];
    }
    delete this->pp_map;
}


list<AggrOp*>* PostProcessors::get_schedule(int r){
    list<AggrOp*>* schedule = new list<AggrOp*>();

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        schedule->push_back(it->second->get_op(r));
    }

    return schedule;
}

list<PostProcessor*>* PostProcessors::available_pps(int r){
    list<PostProcessor*>* avail_pps = new list<PostProcessor*>();
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        if(it->second->is_schedule_empty(r)){
            avail_pps->push_back(it->second);
        }
    }
    return avail_pps;
}

bool PostProcessors::check_pin1_bank_conflict(int r, P_Tile* p_tile){
    list<AggrOp*>* schedule = this->get_schedule(r);
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) != nullptr){
            if (p_tile->bank == (*it)->get_op1()->pout_tile->bank){
                return true;
            }
        }
    }

    delete schedule;
    return false;
}

bool PostProcessors::check_pin2_bank_conflict(int r, P_Tile* p_tile){
    list<AggrOp*>* schedule = this->get_schedule(r);
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) != nullptr){
            if (p_tile->bank == (*it)->get_op2()->pout_tile->bank){
                return true;
            }
        }
    }

    delete schedule;
    return false;
}

bool PostProcessors::check_pout_bank_conflict(int r, P_Tile* p_tile){
    list<AggrOp*>* schedule = this->get_schedule(r);
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) != nullptr){
            if (p_tile->bank == (*it)->pout_tile->bank){
                return true;
            }
        }
    }
    
    delete schedule;
    return false;
}

map<PostProcessor*, Bank*>* PostProcessors::get_pin1_permute(int r){
    auto pin_permute = new map<PostProcessor*, Bank*>();
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->get_op1() == nullptr) continue;
        if(op->get_op1()->pout_tile == nullptr) continue;
        (*pin_permute)[it->second] = op->get_op1()->pout_tile->bank;
    }
    return pin_permute;
}

map<PostProcessor*, Bank*>* PostProcessors::get_pin2_permute(int r){
    auto pin_permute = new map<PostProcessor*, Bank*>();
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->get_op2() == nullptr) continue;
        if(op->get_op2()->pout_tile == nullptr) continue;
        (*pin_permute)[it->second] = op->get_op2()->pout_tile->bank;
    }
    return pin_permute;
}

map<PostProcessor*, Bank*>* PostProcessors::get_pout_permute(int r){
    auto pout_permute = new map<PostProcessor*, Bank*>();
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pout_tile == nullptr) continue;
        if(op->pout_tile->bank == nullptr) continue;
        (*pout_permute)[it->second] = op->pout_tile->bank;
    }
    return pout_permute;    
}

void PostProcessors::init_tile_op(int r){
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        it->second->init_tile_op(r);
    }
}

bool PostProcessors::is_tile_op_done(int r){
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        if( !(it->second->is_tile_op_done(r))){
            return false;
        }
    }
    return true;
}

void PostProcessors::update(){
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        it->second->update();
    }
}

list<P_Tile*>* PostProcessors::get_pin1_tiles(int r){
    list<P_Tile*>* pin1_tiles = new list<P_Tile*>();

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        pin1_tiles->push_back(op->pin1_tile);
    }    

    return pin1_tiles;
}

list<P_Tile*>* PostProcessors::get_pin2_tiles(int r){
    list<P_Tile*>* pin2_tiles = new list<P_Tile*>();

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        pin2_tiles->push_back(op->pin2_tile);
    }    

    return pin2_tiles;
}

list<P_Tile*>* PostProcessors::get_pout_tiles(int r){
    list<P_Tile*>* pout_tiles = new list<P_Tile*>();

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        pout_tiles->push_back(op->pout_tile);
    }    

    return pout_tiles;
}

bool PostProcessors::is_pin1_ready(int r, Dram* dram){
    bool is_ready = true;

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        if (!op->pin1_tile->is_allocated()){
            dram->load_queue->push_front(make_pair(r, op->pin1_tile));
            BOOST_LOG_TRIVIAL(info) << "Pin1 Tile: " << op->pin1_tile->tag << " is missed, placing it in front of the load queue";
            is_ready = false;
        }
    }    

    return is_ready;
}

bool PostProcessors::is_pin2_ready(int r, Dram* dram){
    bool is_ready = true;

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        if (!op->pin2_tile->is_allocated()){
            dram->load_queue->push_front(make_pair(r, op->pin2_tile));
            BOOST_LOG_TRIVIAL(info) << "Pin2 Tile: " << op->pin2_tile->tag << " is missed, placing it in front of the load queue";
            is_ready = false;
        }
    }    

    return is_ready;
}

bool PostProcessors::is_pout_ready(int r, Dram* dram){
    bool is_ready = true;

    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        AggrOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        if (!op->pout_tile->is_allocated()){
            dram->load_queue->push_front(make_pair(r, op->pout_tile));
            BOOST_LOG_TRIVIAL(info) << "PP_pout Tile: " << op->pout_tile->tag << " is missed, placing it in front of the load queue";
            is_ready = false;
        }
    }    

    return is_ready;
}

long PostProcessors::total_no_ops(){
    long no_ops = 0;
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        no_ops += it->second->no_add_ops;
    }
    return no_ops;
}


long PostProcessors::total_sram_read_bytes(){
    long sram_read_bytes = 0;
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        sram_read_bytes += it->second->sram_read_bytes;
    }
    return sram_read_bytes;
}

long PostProcessors::total_sram_write_bytes(){
    long sram_write_bytes = 0;
    for (auto it = this->pp_map->begin(); it != this->pp_map->end(); it++){
        sram_write_bytes += it->second->sram_write_bytes;
    }
    return sram_write_bytes;
}
