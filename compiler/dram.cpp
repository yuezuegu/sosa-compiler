

#include "dram.hpp"




Dram::Dram(float bandwidth){
    this->bandwidth = bandwidth; //bytes per cycle

    this->memory_queue = new queue<Tile*>();

}

void Dram::update(){

    if (this->memory_queue->empty()) return;

    Tile* front = this->memory_queue->front();
    while (front->is_allocated()){
        this->memory_queue->pop();
        if (this->memory_queue->empty()) return;
        
        front = this->memory_queue->front();
    }

    front->fetch_from_memory(this->bandwidth);
}