
#include "bank.hpp"
#include "tiles.hpp"


Bank::Bank(int id, data_type type){
    this->id = id;
    this->type = type;
};

Bank::~Bank(){
    
}


int Bank::alloc_tile(Tile* tile){
    int addr = this->next_virt_addr;
    this->allocated_tiles[tile] = addr;
    this->next_virt_addr += tile->get_mem_height();
    return addr;
}

Banks::Banks(int no_banks){
    this->no_banks = no_banks;
    this->x_banks = new list<Bank*>();
    this->w_banks = new list<Bank*>();
    this->p_banks = new list<Bank*>();
    for(int i = 0; i < no_banks; i++){
        this->x_banks->push_back(new Bank(i, data_type::X));
    }
    for(int i = 0; i < no_banks; i++){
        this->w_banks->push_back(new Bank(i, data_type::W));
    }
    for(int i = 0; i < no_banks; i++){
        this->p_banks->push_back(new Bank(i, data_type::P));
    }
};

Banks::~Banks(){
    for (auto it = this->x_banks->begin(); it != this->x_banks->end(); it++){
        delete *it;
    }
    delete this->x_banks;

    for (auto it = this->w_banks->begin(); it != this->w_banks->end(); it++){
        delete *it;
    }
    delete this->w_banks;

    for (auto it = this->p_banks->begin(); it != this->p_banks->end(); it++){
        delete *it;
    }
    delete this->p_banks;
}

list<Bank*>* Banks::get_x_banks(){
    return this->x_banks;
};

list<Bank*>* Banks::get_w_banks(){
    return this->w_banks;
};

list<Bank*>* Banks::get_p_banks(){
    return this->p_banks;
};



Bank* Banks::get_bank_by_id(int id, data_type type){
    if (type == data_type::X){
        auto it = this->x_banks->begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
    else if (type == data_type::W){
        auto it = this->w_banks->begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
    else{
        auto it = this->p_banks->begin();
        
        for (int i = 0; i < id; i++){
            it++;
        }

        return *it;
    }
}