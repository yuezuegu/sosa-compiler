
#include "bank.hpp"
#include "tiles.hpp"

Bank::Bank(int id, data_type type, int capacity){
    this->id = id;
    this->type = type;
    this->capacity = capacity;
    this->capacity_used = 0;
}

Bank::~Bank(){
    
}

bool Bank::alloc_tile(Tile* tile){
    if (this->capacity_used + tile->memory_size <= this->capacity){
        this->capacity_used += tile->memory_size;
        return true;
    }
    else{
        return false;
    }
}

void Bank::free_tile(Tile* tile){
    this->capacity_used -= tile->memory_size;
    if (this->capacity_used < 0){
        throw runtime_error("Usage cannot be negative, something is wrong!");
    }
}

Banks::Banks(int no_banks, int bank_size){
    this->no_banks = no_banks;
    this->x_banks.resize(this->no_banks);
    this->w_banks.resize(this->no_banks);
    this->p_banks.resize(this->no_banks);

    for(int i = 0; i < no_banks; i++){
        this->x_banks[i] = new Bank(i, data_type::X, bank_size);
    }
    for(int i = 0; i < no_banks; i++){
        this->w_banks[i] = new Bank(i, data_type::W, bank_size);
    }
    for(int i = 0; i < no_banks; i++){
        this->p_banks[i] = new Bank(i, data_type::P, bank_size);
    }
}

Banks::~Banks(){
    for(int i = 0; i < no_banks; i++){
        delete this->x_banks[i];
        delete this->w_banks[i];
        delete this->p_banks[i];
    }
}

vector<Bank*> Banks::get_x_banks(){
    return this->x_banks;
}

vector<Bank*> Banks::get_w_banks(){
    return this->w_banks;
}

vector<Bank*> Banks::get_p_banks(){
    return this->p_banks;
}

Bank* Banks::get_bank_by_id(int id, data_type type){
    if (type == data_type::X){
        return this->x_banks[id];
    }
    else if (type == data_type::W){
        return this->w_banks[id];
    }
    else{
        return this->p_banks[id];
    }
}