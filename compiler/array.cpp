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

    this->arr_state = ARR_STATE::idle;
    this->x_tile = nullptr;
    this->pin_tile = nullptr;
    this->pout_tile = nullptr;
    this->exec_cnt = 0;
    this->curr_op = nullptr;

    this->no_macs = 0;
    this->sram_read_bytes = 0;
    this->sram_write_bytes = 0;
}

Array::~Array(){
    
}

void Array::update(){
    if (this->buf_state == BUF_STATE::buffering) {
        if (this->buf_cnt < get<0>(this->next_w_tile->dims)){
            assert(this->next_w_tile->is_allocated() && "Next_W tile is not on sram");
            this->buf_cnt++;
        }
        else{
            this->buf_state = BUF_STATE::buffered;
        }
    }

    if (this->arr_state == ARR_STATE::processing){
        if (this->exec_cnt < get<0>(this->x_tile->dims)){
            assert(this->x_tile->is_allocated() && "X tile is not on sram");
            assert(this->curr_w_tile->is_allocated() && "Curr_W_tile is not on sram");
            assert(this->pout_tile->is_allocated() && "Pout is not on sram");
            if (this->pin_tile != nullptr){
                assert(this->pin_tile->is_allocated() && "Pin is not on sram");
            }
            this->exec_cnt++;
        }
        else{
            this->arr_state = ARR_STATE::done;
            this->curr_op->retire();
        }
    }
}

void Array::init_weight_buffering(int r){
    if (this->get_op(r) == nullptr) return;
    
    if ( this->buf_state == BUF_STATE::buffering) return;

    this->buf_cnt = 0;
    this->buf_state = BUF_STATE::buffering;
    this->next_w_tile = this->get_op(r)->w_tile;
    this->sram_read_bytes += this->next_w_tile->memory_size;
}

bool Array::is_weight_buffered(int r){
    if (this->get_op(r) == nullptr) return true;

    if ( this->buf_state == BUF_STATE::buffered){
        if (this->next_w_tile == this->get_op(r)->w_tile){
            return true;
        }
    }
    
    return false;
}

void Array::init_tile_op(int r){
    if (this->get_op(r) == nullptr) return;

    if ( this->arr_state == ARR_STATE::processing){
        throw runtime_error("Array is already in processing state");
    }

    if ( this->buf_state == BUF_STATE::buffering){
        throw runtime_error("Buffering is not done.");
    }

    this->exec_cnt = 0;
    this->arr_state = ARR_STATE::processing;

    this->curr_op = this->get_op(r);
    this->x_tile = this->curr_op->x_tile;
    this->sram_read_bytes += this->x_tile->memory_size;

    this->pout_tile = this->curr_op->pout_tile;
    this->sram_write_bytes += this->pout_tile->memory_size;

    this->no_macs += get<0>(this->x_tile->dims) * get<1>(this->x_tile->dims) * get<1>(this->pout_tile->dims);

    if (this->curr_op->pin_op != nullptr){
        this->pin_tile = this->curr_op->pin_op->pout_tile;
        this->sram_read_bytes += this->pin_tile->memory_size;
    }
    else{
        this->pin_tile = nullptr;
    }
    
    this->curr_w_tile = this->next_w_tile;
    this->next_w_tile = nullptr;
}

bool Array::is_tile_op_done(int r){
    if (this->get_op(r) == nullptr) return true;

    if (this->x_tile != this->get_op(r)->x_tile){
        throw runtime_error("Wrong tile is loaded, something is wrong.");
    }

    return this->arr_state == ARR_STATE::done;
}

bool Array::is_idle(){
    return this->arr_state == ARR_STATE::idle || this->arr_state == ARR_STATE::done;
}

bool Array::is_schedule_empty(int r){
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
        if(it->second->is_schedule_empty(r)){
            avail_arrays->push_back(it->second);
        }
    }
    return avail_arrays;
}

map<Array*, Bank*>* Arrays::get_x_permute(int r){
    map<Array*, Bank*>* x_permute = new map<Array*, Bank*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->x_tile == nullptr) continue;
        if(op->x_tile->bank == nullptr) continue;
        (*x_permute)[it->second] = op->x_tile->bank;
    }
    return x_permute;
}

map<Array*, Bank*>* Arrays::get_w_permute(int r){
    map<Array*, Bank*>* w_permute = new map<Array*, Bank*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->w_tile == nullptr) continue;
        if(op->w_tile->bank == nullptr) continue;
        (*w_permute)[it->second] = op->w_tile->bank;
    }
    return w_permute;
}

map<Array*, Bank*>* Arrays::get_pout_permute(int r){
    map<Array*, Bank*>* pout_permute = new map<Array*, Bank*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pout_tile == nullptr) continue;
        if(op->pout_tile->bank == nullptr) continue;
        (*pout_permute)[it->second] = op->pout_tile->bank;
    }
    return pout_permute;
}

map<Array*, Bank*>* Arrays::get_pin_permute(int r){
    map<Array*, Bank*>* pin_permute = new map<Array*, Bank*>();
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if(op == nullptr) continue;
        if(op->pin_op == nullptr) continue;
        (*pin_permute)[it->second] = op->pin_op->pout_tile->bank;
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

list<X_Tile*>* Arrays::get_x_tiles(int r){
    list<X_Tile*>* x_tiles = new list<X_Tile*>();

    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        x_tiles->push_back(op->x_tile);
    }    

    return x_tiles;
}

list<W_Tile*>* Arrays::get_w_tiles(int r){
    list<W_Tile*>* w_tiles = new list<W_Tile*>();

    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        w_tiles->push_back(op->w_tile);
    }    

    return w_tiles;
}

list<P_Tile*>* Arrays::get_pout_tiles(int r){
    list<P_Tile*>* pout_tiles = new list<P_Tile*>();

    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        pout_tiles->push_back(op->pout_tile);
    }    

    return pout_tiles;
}

list<P_Tile*>* Arrays::get_pin_tiles(int r){
    list<P_Tile*>* pin_tiles = new list<P_Tile*>();

    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        MultOp* op = it->second->get_op(r);
        if (op == nullptr) continue;
        if (op->pin_op == nullptr) continue;
        pin_tiles->push_back(op->pin_op->pout_tile);
    }    

    return pin_tiles;
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

void Arrays::init_tile_op(int r){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        it->second->init_tile_op(r);
    }
}

bool Arrays::is_tile_op_done(int r){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        if( !(it->second->is_tile_op_done(r))){
            return false;
        }
    }
    return true;
}

void Arrays::update(){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        it->second->update();
    }
}

bool Arrays::is_idle(){
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        if( !(it->second->is_idle())){
            return false;
        }
    }
    return true;    
}

long Arrays::total_no_ops(){
    long no_ops = 0;
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        no_ops += 2*it->second->no_macs;
    }
    return no_ops;
}

long Arrays::total_sram_read_bytes(){
    long sram_read_bytes = 0;
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        sram_read_bytes += it->second->sram_read_bytes;
    }
    return sram_read_bytes;
}

long Arrays::total_sram_write_bytes(){
    long sram_write_bytes = 0;
    for (auto it = this->array_map->begin(); it != this->array_map->end(); it++){
        sram_write_bytes += it->second->sram_write_bytes;
    }
    return sram_write_bytes;
}