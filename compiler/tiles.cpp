
#include <cassert>

#include "tiles.hpp"
#include "bank.hpp"
#include "ops.hpp"

X_Tile::X_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims){
    this->layer_name = layer_name;
    this->type = data_type::X;
    this->id = id;
    this->dims = dims;
    this->is_allocated_on_sram = false;
    this->bank_id = -1;
    this->virt_bank_addr = -1;
    this->phys_bank_addr = -1;
    this->bank = nullptr;
    this->input_of = new list<Op*>();
};

W_Tile::W_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims){
    this->layer_name = layer_name;
    this->type = data_type::W;
    this->id = id;
    this->dims = dims;
    this->is_allocated_on_sram = false;
    this->bank_id = -1;
    this->virt_bank_addr = -1;
    this->phys_bank_addr = -1;
    this->bank = nullptr;
    this->input_of = new list<Op*>();
};

P_Tile::P_Tile(string layer_name, tuple<int, int, int> id, tuple<int, int> dims){
    this->layer_name = layer_name;
    this->type = data_type::P;
    this->id = id;
    this->dims = dims;
    this->is_allocated_on_sram = false;
    this->bank_id = -1;
    this->virt_bank_addr = -1;
    this->phys_bank_addr = -1;
    this->bank = nullptr;
    this->input_of = new list<Op*>();
};

void Tile::assign_bank(Bank* bank){
    assert((this->type == bank->type && "Tile type and bank type do not match!"));
    this->bank = bank;
    this->bank_id = bank->id;
    this->virt_bank_addr = bank->alloc_tile(this);

    
};


int Tile::get_mem_height(){
    return get<0>(this->dims);
}

int Tile::get_mem_width(){
    return get<1>(this->dims);
}

bool Tile::is_allocated(){
    return this->is_allocated_on_sram;
}








