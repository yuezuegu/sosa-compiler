
#include <cassert>

#include "tiles.hpp"
#include "bank.hpp"

Tile::Tile(string layer_name, data_type type, tuple<int, int> id, tuple<int, int> dims){
    this->layer_name = layer_name;
    this->type = type;
    this->id = id;
    this->dims = dims;
    this->is_allocated_ = false;
    this->bank_id = -1;
    this->virt_bank_addr = -1;
    this->phys_bank_addr = -1;
};


void Tile::assign_bank(Bank* bank){
    assert((this->type == bank->type && "Tile type and bank type do not match!"));
    this->bank_id = bank->id;
    this->virt_bank_addr = bank->get_next_virt_addr();
    this->is_allocated_ = true;

    bank->alloc_tile(this);
};


int Tile::get_mem_height(){
    return get<0>(this->dims);
}

int Tile::get_mem_width(){
    return get<1>(this->dims);
}

bool Tile::is_allocated(){
    return this->is_allocated_;
}








