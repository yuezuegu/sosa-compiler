#include "array.hpp"

Array::Array(int id, int no_rows, int no_cols){
    this->id = id;
    this->no_rows = no_rows;
    this->no_cols = no_cols;
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
    }
    
    if (sch->second == nullptr){
        this->schedule[r] = op;
        op->assign_to_array(r, this);
    }

    throw runtime_error("Cannot assign op to array");
}

Arrays::Arrays(int no_arrays, int no_rows, int no_cols){
    this->no_arrays = no_arrays;

    for (int i = 0; i < no_arrays; i++){
        this->array_map[i] = new Array(i, no_rows, no_cols);
    }
}

list<MultOp*> Arrays::get_schedule(int r){
    list<MultOp*> schedule;
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        schedule.push_back(it->second->get_op(r));
    }
    return schedule;
};

list<Array*> Arrays::available_arrays(int r){
    list<Array*> avail_arrays;
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        if(it->second->is_idle(r)){
            avail_arrays.push_back(it->second);
        }
    }
    return avail_arrays;
};

map<Bank*, Array*> Arrays::get_x_permute(int r){
    map<Bank*, Array*> x_permute; //key: bank_id, value: array_id
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->x_tile == nullptr) continue;
        if(!op->x_tile->is_allocated()) continue;
        x_permute[op->x_tile->bank] = it->second;
    }
    return x_permute;
};

map<Bank*, Array*> Arrays::get_w_permute(int r){
    map<Bank*, Array*> w_permute; //key: bank_id, value: array_id
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->w_tile == nullptr) continue;
        if(!op->w_tile->is_allocated()) continue;
        w_permute[op->w_tile->bank] = it->second;
    }
    return w_permute;
};

map<Bank*, Array*> Arrays::get_pout_permute(int r){
    map<Bank*, Array*> pout_permute; //key: bank_id, value: array_id
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pout_tile == nullptr) continue;
        if(!op->pout_tile->is_allocated()) continue;
        pout_permute[op->pout_tile->bank] = it->second;
    }
    return pout_permute;
};

map<Bank*, Array*> Arrays::get_pin_permute(int r){
    map<Bank*, Array*> pin_permute; //key: bank_id, value: array_id
    for (auto it = this->array_map.begin(); it != this->array_map.end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pin_op == nullptr) continue;
        pin_permute[op->pin_op->pout_tile->bank] = it->second;
    }
    return pin_permute;
};

bool Arrays::check_x_bank_conflict(int r, X_Tile* x_tile){
    list<MultOp*> schedule = this->get_schedule(r);
    for (auto it = schedule.begin(); it != schedule.end(); it++){
        if ((*it) != nullptr){
            if ((*it)->x_tile != nullptr){
                if (x_tile->bank == (*it)->x_tile->bank){
                    return true;
                }
            }
        }
    }
    return false;
};

bool Arrays::check_w_bank_conflict(int r, W_Tile* w_tile){
    list<MultOp*> schedule = this->get_schedule(r);
    for (auto it = schedule.begin(); it != schedule.end(); it++){
        if ((*it) != nullptr){
            if ((*it)->w_tile != nullptr){
                if (w_tile->bank == (*it)->w_tile->bank){
                    return true;
                }
            }
        }
    }
    return false;
};

bool Arrays::check_pout_bank_conflict(int r, P_Tile* p_tile){
    list<MultOp*> schedule = this->get_schedule(r);
    for (auto it = schedule.begin(); it != schedule.end(); it++){
        if ((*it) != nullptr){
            if ((*it)->pout_tile != nullptr){
                if (p_tile->bank == (*it)->pout_tile->bank){
                    return true;
                }
            }
        }
    }
    return false;
};

bool Arrays::check_pin_bank_conflict(int r, P_Tile* p_tile){
    list<MultOp*> schedule = this->get_schedule(r);
    for (auto it = schedule.begin(); it != schedule.end(); it++){
        if ((*it) != nullptr){
            if ((*it)->pin_op != nullptr){
                if (p_tile->bank == (*it)->pin_op->pout_tile->bank){
                    return true;
                }
            }
        }
    }
    return false;
};