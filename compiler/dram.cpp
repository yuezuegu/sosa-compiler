

#include "dram.hpp"




Dram::Dram(float bandwidth){
    this->bandwidth = bandwidth; //bytes per cycle

    this->request_queue = new list<Tile*>();
    this->evict_queue = new list<Tile*>();
    this->prefetch_iter = this->request_queue->begin();
}

void Dram::update(){
    if (this->request_queue->empty()) return;

    while((*this->prefetch_iter)->is_allocated() && this->prefetch_iter != this->request_queue->end()){
        this->prefetch_iter++;
    }

    if (this->prefetch_iter == this->request_queue->end()) return;

    (*this->prefetch_iter)->fetch_from_memory(this->bandwidth);
}