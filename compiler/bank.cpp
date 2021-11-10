
#include "bank.hpp"
#include "tiles.hpp"

#include <boost/log/trivial.hpp>

Bank::Bank(int id, data_type type, int capacity){
    this->id = id;
    this->type = type;
    this->capacity = capacity;
    this->capacity_used = 0;
    this->evict_queue = new list<pair<int, Tile*>>();
    this->write_back_queue = new list<Tile*>();
    this->spawn_queue = new list<pair<int, Tile*>>();
}

Bank::~Bank(){
    
}


int Bank::evict_queue_size(){
    return accumulate(this->evict_queue->begin(), this->evict_queue->end(), 0, [](int s, pair<int, Tile*> p){return p.second->memory_size + s;});
}

void Bank::spawn(int r){
    if (this->spawn_queue->empty()) return;

    pair<int,Tile*> front = this->spawn_queue->front();

    while (front.first <= r){
        if(!front.second->is_spawn_){
            if(!front.second->allocate_on_sram(r, front.first)){
                return;
            }
            //this->push_evict_queue(front.first, front.second);
        }

        this->spawn_queue->pop_front();
        if (this->spawn_queue->empty()) return;
        
        front = this->spawn_queue->front();
    }
}


bool Bank::alloc_tile(int target_round, Tile* tile){
    assert( (tile->memory_size <= this->capacity) && "Tile size is larger than sram size" );

    if (this->capacity_used + tile->memory_size <= this->capacity){
        this->capacity_used += tile->memory_size;
        this->push_evict_queue(target_round, tile);
        tile->is_spawn_ = true;
        

        BOOST_LOG_TRIVIAL(info) << "Bank: " << this->id << " new tile of type "<< PRINT_TYPE(tile->type) << " allocated, usage: " << this->capacity_used; //<< " evict_queue_size: " << this->evict_queue_size();
        return true;
    }
    else{
        return false;
    }
}

// void Bank::garbage_collect(int r){
//     auto it = this->evict_queue->begin();
//     while (it != this->evict_queue->end()){
//         if (it->first > r){
//             return;
//         }

//         if (it->second->is_allocated()){
//             it->second->remove_from_sram();
//         }
        
//         this->evict_queue->erase(it++);
//     }
// }

bool Bank::evict(int r){
    if (this->evict_queue->empty()) return false;

    auto front_it = this->evict_queue->begin();
    if (front_it->first >= r) return false;

    // if (!front_it->second->is_allocated()){
    //     this->evict_queue->erase(front_it);  
    //     return true;
    // }

    front_it->second->remove_from_sram();
    this->evict_queue->erase(front_it);
    this->free_tile(front_it->second);
    return true;

}

void Bank::free_tile(Tile* tile){
    this->capacity_used -= tile->memory_size;
    
    //assert(this->capacity_used == this->evict_queue_size());

    BOOST_LOG_TRIVIAL(info) << "Bank: " << this->id << " tile of type "<< PRINT_TYPE(tile->type) <<" is freed, usage: " << this->capacity_used; // << " evict_queue_size: " << this->evict_queue_size();
    if (this->capacity_used < 0){
        throw runtime_error("Usage cannot be negative, something is wrong!");
    }
}

void Bank::push_evict_queue(int r, Tile* tile){
    int target_round = r;
    for (auto it = this->evict_queue->begin(); it != this->evict_queue->end(); it++){
        if (tile == it->second){
            if (it->first > target_round){
                target_round = it->first;
            }

            this->evict_queue->erase(it);
            break;
        }
    }
    this->evict_queue->push_back(make_pair(target_round, tile));
    //assert(this->capacity_used == this->evict_queue_size());
}

Banks::Banks(int no_banks, int bank_size){
    this->no_banks = no_banks;
    this->x_banks = new list<Bank*>();
    this->w_banks = new list<Bank*>();
    this->p_banks = new list<Bank*>();
    for(int i = 0; i < no_banks; i++){
        this->x_banks->push_back(new Bank(i, data_type::X, bank_size));
    }
    for(int i = 0; i < no_banks; i++){
        this->w_banks->push_back(new Bank(i, data_type::W, bank_size));
    }
    for(int i = 0; i < no_banks; i++){
        this->p_banks->push_back(new Bank(i, data_type::P, bank_size));
    }
}

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
}

list<Bank*>* Banks::get_w_banks(){
    return this->w_banks;
}

list<Bank*>* Banks::get_p_banks(){
    return this->p_banks;
}

void Banks::garbage_collect(int r){
    // for (auto it = this->x_banks->begin(); it != this->x_banks->end(); it++){
    //     (*it)->garbage_collect(r);
    // }
    // for (auto it = this->w_banks->begin(); it != this->w_banks->end(); it++){
    //     (*it)->garbage_collect(r);
    // }
    // for (auto it = this->p_banks->begin(); it != this->p_banks->end(); it++){
    //     (*it)->garbage_collect(r);
    // }
}

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

void Banks::spawn(int r){
    for (auto it = this->x_banks->begin(); it != this->x_banks->end(); it++){
        (*it)->spawn(r);
    }
    for (auto it = this->w_banks->begin(); it != this->w_banks->end(); it++){
        (*it)->spawn(r);
    }
    for (auto it = this->p_banks->begin(); it != this->p_banks->end(); it++){
        (*it)->spawn(r);
    }
}

void Banks::print_usage(){
    cout << "x_banks:" << endl;
    for (auto it = this->x_banks->begin(); it != this->x_banks->end(); it++){
        cout << "bank " << (*it)->id << " : " << (*it)->capacity_used << " / " << (*it)->capacity << endl;
    }
    cout << "w_banks:" << endl;
    for (auto it = this->w_banks->begin(); it != this->w_banks->end(); it++){
        cout << "bank " << (*it)->id << " : " << (*it)->capacity_used << " / " << (*it)->capacity << endl;
    }
    cout << "p_banks:" << endl;
    for (auto it = this->p_banks->begin(); it != this->p_banks->end(); it++){
        cout << "bank " << (*it)->id << " : " << (*it)->capacity_used << " / " << (*it)->capacity << endl;
    }
    cout << flush;
}
