
#include <cassert>

#include "tiles.hpp"
#include "bank.hpp"
#include "ops.hpp"

X_Tile::X_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision){
    this->layer_name = layer_name;
    this->type = data_type::X;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = get<0>(dims) * get<1>(dims) * precision;
    this->bytes_fetched_from_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
}

W_Tile::W_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision){
    this->layer_name = layer_name;
    this->type = data_type::W;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = get<0>(dims) * get<1>(dims) * precision;
    this->bytes_fetched_from_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
}

P_Tile::P_Tile(string layer_name, tuple<int, int, int> id, tuple<int, int> dims, int precision){
    this->layer_name = layer_name;
    this->type = data_type::P;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = get<0>(dims) * get<1>(dims) * precision;
    this->bytes_fetched_from_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
}

void Tile::assign_bank(Bank* bank){
    assert((this->type == bank->type && "Tile type and bank type do not match!"));
    this->bank = bank;
}

void Tile::try_free(){
    for(auto it = this->input_of->begin(); it != this->input_of->end(); it++){
        if (!(*it)->retired){
            return;
        }
    }

    this->is_allocated_on_sram = false;
    this->bytes_fetched_from_memory = 0;
    this->bank->free_tile(this);
}

int Tile::fetch_from_memory(int bytes){
    int usage;

    if (this->bytes_fetched_from_memory == 0){
        if (!this->bank->alloc_tile(this)){
            return 0; 
        }
    }

    if (this->bytes_fetched_from_memory+bytes>=this->memory_size){
        usage = this->memory_size - this->bytes_fetched_from_memory;
        this->bytes_fetched_from_memory = this->memory_size;
        this->is_allocated_on_sram = true;
        return usage;
    }
    else{
        this->bytes_fetched_from_memory += bytes;
        usage = bytes;
        return usage;
    }
}

int Tile::get_mem_height(){
    return get<0>(this->dims);
}

int Tile::get_mem_width(){
    return get<1>(this->dims);
}

bool Tile::is_allocated(){
    return this->is_allocated_on_sram;
}








