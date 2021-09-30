

#include "post_processor.hpp"



PostProcessor::PostProcessor(int id){
    this->id = id;
    this->last_no_round = 0;
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

    throw runtime_error("Cannot assign op to array");
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
