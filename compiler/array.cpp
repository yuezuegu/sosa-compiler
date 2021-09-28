#include "array.hpp"

Array::Array(int id, int no_rows, int no_cols){
    this->id = id;
    this->no_rows = no_rows;
    this->no_cols = no_cols;
    this->last_no_round = 0;

    // Make this parametric in terms of local broadcast and fanin
    this->pipeline_cycles = 2;

    this->curr_w_tile = nullptr;
    this->next_w_tile = nullptr;
    this->buf_state = BUF_STATE::empty;
    this->buf_cnt = 0;
}

Array::~Array(){
    
}

void Array::update(){
    if (this->buf_state == BUF_STATE::buffering) {
        if (this->buf_cnt < get<0>(this->next_w_tile->dims)){
            // FIXME This statement has no effect?
            this->buf_cnt;
        }
        else{
            this->buf_state = BUF_STATE::buffered;
            this->curr_w_tile = this->next_w_tile;
            this->next_w_tile = nullptr;
        }
    }
}

void Array::init_weight_buffering(int r){
    if (this->get_op(r) == nullptr) return;
    
    if ( this->buf_state == BUF_STATE::buffering){
        throw runtime_error("Array is already in buffering state");
    }

    this->buf_cnt = 0;
    this->buf_state = BUF_STATE::buffering;
    this->next_w_tile = this->get_op(r)->w_tile;
}

bool Array::is_weight_buffered(int r){
    // FIXME Did you mean: return true
    if (this->get_op(r) == nullptr) true;

    if ( this->buf_state == BUF_STATE::buffered){
        if (this->next_w_tile == this->get_op(r)->w_tile){
            return true;
        }
    }
    
    return false;
}


bool Array::is_idle(int r){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        return true;
    }

    if (sch->second == nullptr){
        return true;
    }  

    return false;
}

MultOp* Array::get_op(int r){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        return nullptr;
    }

    return sch->second;
}

void Array::assign_op(int r, MultOp* op){
    auto sch = this->schedule.find(r);
    if (sch == this->schedule.end()){
        this->schedule[r] = op;
        op->assign_to_array(r, this);
        if (r > this->last_no_round){
            this->last_no_round = r;
        }
        return;
    }
    
    if (sch->second == nullptr){
        this->schedule[r] = op;
        op->assign_to_array(r, this);
        return;
    }

    throw runtime_error("Cannot assign op to array");
}

Arrays::Arrays(int no_arrays, int no_rows, int no_cols){
    this->no_arrays = no_arrays;
    this->array_map = new map<int, Array*>();

    for (int i = 0; i < no_arrays; i++){
        (*this->array_map)[i] = new Array(i, no_rows, no_cols);
    }
}

Arrays::~Arrays(){
    for (int i = 0; i < no_arrays; i++){
        delete (*this->array_map)[i];
    }
    delete this->array_map;
}

list<MultOp*>* Arrays::get_schedule(int r){
    list<MultOp*>* schedule = new list<MultOp*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        schedule->push_back(it->second->get_op(r));
    }
    return schedule;
}

list<Array*>* Arrays::available_arrays(int r){
    list<Array*>* avail_arrays = new list<Array*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        if(it->second->is_idle(r)){
            avail_arrays->push_back(it->second);
        }
    }
    return avail_arrays;
}

map<Bank*, Array*>* Arrays::get_x_permute(int r){
    map<Bank*, Array*>* x_permute = new map<Bank*, Array*>(); //key: bank_id, value: array_id
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->x_tile == nullptr) continue;
        if(op->x_tile->bank == nullptr) continue;
        (*x_permute)[op->x_tile->bank] = it->second;
    }
    return x_permute;
}

map<Bank*, Array*>* Arrays::get_w_permute(int r){
    map<Bank*, Array*>* w_permute = new map<Bank*, Array*>(); //key: bank_id, value: array_id
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->w_tile == nullptr) continue;
        if(op->w_tile->bank == nullptr) continue;
        (*w_permute)[op->w_tile->bank] = it->second;
    }
    return w_permute;
}

map<Bank*, Array*>* Arrays::get_pout_permute(int r){
    map<Bank*, Array*>* pout_permute = new map<Bank*, Array*>(); //key: bank_id, value: array_id
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pout_tile == nullptr) continue;
        if(op->pout_tile->bank == nullptr) continue;
        (*pout_permute)[op->pout_tile->bank] = it->second;
    }
    return pout_permute;
}

map<Bank*, Array*>* Arrays::get_pin_permute(int r){
    map<Bank*, Array*>* pin_permute = new map<Bank*, Array*>(); //key: bank_id, value: array_id
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pin_op == nullptr) continue;
        (*pin_permute)[op->pin_op->pout_tile->bank] = it->second;
    }
    return pin_permute;
}

bool Arrays::check_x_bank_conflict(int r, X_Tile* x_tile){
    unique_ptr< list<MultOp*> > schedule (this->get_schedule(r));
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) == nullptr) continue;
        if ((*it)->x_tile == nullptr) continue;
        if ((*it)->x_tile == x_tile) continue;
        if ((*it)->x_tile->bank != x_tile->bank) continue;
        return true;
    }
    return false;
}

bool Arrays::check_w_bank_conflict(int r, W_Tile* w_tile){
    unique_ptr< list<MultOp*> > schedule (this->get_schedule(r));
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) == nullptr) continue;
        if ((*it)->w_tile == nullptr) continue;
        if ((*it)->w_tile == w_tile) continue;
        if ((*it)->w_tile->bank != w_tile->bank) continue;
        return true;
    }
    return false;
}

bool Arrays::check_pout_bank_conflict(int r, P_Tile* p_tile){
    list<MultOp*>* schedule = this->get_schedule(r);
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) != nullptr){
            if ((*it)->pout_tile != nullptr){
                if (p_tile->bank == (*it)->pout_tile->bank){
                    return true;
                }
            }
        }
    }
    delete schedule;
    return false;
}

bool Arrays::check_pin_bank_conflict(int r, P_Tile* p_tile){
    list<MultOp*>* schedule = this->get_schedule(r);
    for (auto it = schedule->begin(); it != schedule->end(); it++){
        if ((*it) != nullptr){
            if ((*it)->pin_op != nullptr){
                if (p_tile->bank == (*it)->pin_op->pout_tile->bank){
                    return true;
                }
            }
        }
    }
    delete schedule;
    return false;
}


bool Arrays::is_weights_buffered(int r){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        if( !(it->second->is_weight_buffered(r))){
            return false;
        }
    }
    return true;
}

void Arrays::init_weight_buffering(int r){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        it->second->init_weight_buffering(r);
    }
}

void Arrays::update(){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        it->second->update();
    }
}