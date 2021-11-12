
#include <cassert>

#include "tiles.hpp"
#include "bank.hpp"
#include "ops.hpp"

X_Tile::X_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision, int memory_size){
    this->layer_name = layer_name;
    this->type = data_type::X;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = memory_size;
    this->bytes_fetched_from_memory = 0;
    this->bytes_written_to_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
    this->output_of = nullptr;
    
    this->is_spawn_ = false;

    this->tag = "" + this->layer_name + ":" + to_string(get<0>(this->id)) + "-" + to_string(get<1>(this->id));
}

W_Tile::W_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision, int memory_size){
    this->layer_name = layer_name;
    this->type = data_type::W;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = memory_size;
    this->bytes_fetched_from_memory = 0;
    this->bytes_written_to_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
    this->output_of = nullptr;

    this->is_spawn_ = false;

    this->tag = "" + this->layer_name + ":" + to_string(get<0>(this->id)) + "-" + to_string(get<1>(this->id));
}

P_Tile::P_Tile(string layer_name, tuple<int, int, int> id, tuple<int, int> dims, int precision, int memory_size){
    this->layer_name = layer_name;
    this->type = data_type::P;
    this->id = id;
    this->dims = dims;
    this->precision = precision;
    this->memory_size = memory_size;
    this->bytes_fetched_from_memory = 0;
    this->bytes_written_to_memory = 0;

    this->is_allocated_on_sram = false;

    this->bank = nullptr;
    this->input_of = new list<Op*>();
    this->output_of = nullptr;
    
    this->is_spawn_ = false;
    this->tag = "" + this->layer_name + ":" + to_string(get<0>(this->id)) + "-" + to_string(get<1>(this->id)) + "-" + to_string(get<2>(this->id));
}

void Tile::assign_bank(Bank* bank){
    assert((this->type == bank->type && "Tile type and bank type do not match!"));
    this->bank = bank;
}

void Tile::remove_from_sram(){
    this->is_allocated_on_sram = false;
    this->bytes_fetched_from_memory = 0;
    BOOST_LOG_TRIVIAL(info) << "Tile " << this->tag <<" is removed from sram";
}

float Tile::write_to_memory(float bytes){
    float usage;

    if (this->bytes_written_to_memory+bytes>=this->memory_size){
        usage = this->memory_size - this->bytes_written_to_memory;
        this->bytes_written_to_memory = this->memory_size;

        this->bank->write_back_queue->pop_front();

        return usage;
    }
    else{
        this->bytes_written_to_memory += bytes;
        usage = bytes;
        return usage;
    }
}

float Tile::fetch_from_memory(int curr_round, int target_round, float bytes){
    float usage;

    if (this->bytes_fetched_from_memory == 0){
        while(!this->bank->alloc_tile(target_round, this)){
            if(!this->bank->evict(curr_round)){
                return 0;
            }
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

bool Tile::allocate_on_sram(int curr_round, int target_round){
    while(!this->bank->alloc_tile(target_round, this)){
        if(!this->bank->evict(curr_round)){
            return false;
        }
    }

    this->is_allocated_on_sram = true;
    BOOST_LOG_TRIVIAL(info) << "Tile " << this->tag <<" is allocated on sram";
    return true;
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

string P_Tile::get_id_str(){
    string id_str; 
    id_str += to_string(get<0>(this->id));
    id_str += "-";
    id_str += to_string(get<1>(this->id));
    id_str += "-";
    id_str += to_string(get<2>(this->id));
    return id_str;
}

