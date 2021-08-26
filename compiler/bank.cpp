
#include "bank.hpp"
#include "tiles.hpp"


Bank::Bank(int id, data_type type, int size){
    this->id = id;
    this->type = type;
    this->size = size;
};

int Bank::get_next_virt_addr(){
    return this->next_virt_addr;
}

void Bank::alloc_tile(Tile* tile){
    this->allocated_tiles[tile] = this->next_virt_addr;
    this->next_virt_addr += tile->get_mem_height();
}

Banks::Banks(int no_banks, int size){
    this->no_banks = no_banks;
    for(int i = 0; i < no_banks; i++){
        this->x_banks.push_back(new Bank(i, data_type::X, size));
    }
    for(int i = 0; i < no_banks; i++){
        this->w_banks.push_back(new Bank(i, data_type::W, size));
    }
    for(int i = 0; i < no_banks; i++){
        this->p_banks.push_back(new Bank(i, data_type::P, size));
    }
};

list<Bank*> Banks::get_x_banks(){
    return this->x_banks;
};

list<Bank*> Banks::get_w_banks(){
    return this->w_banks;
};

list<Bank*> Banks::get_p_banks(){
    return this->p_banks;
};



Bank* Banks::get_bank_by_id(int id, data_type type){
    if (type == data_type::X){
        auto it = this->x_banks.begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
    else if (type == data_type::W){
        auto it = this->w_banks.begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
    else{
        auto it = this->p_banks.begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
}