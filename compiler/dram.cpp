

#include "dram.hpp"




Dram::Dram(float bandwidth){
    this->bandwidth = bandwidth; //bytes per cycle

    this->load_queue = new list<pair<int, Tile*>>();
}

void Dram::update(list<Bank*>* p_banks){
    float bw_used = 0;

    for (auto it = p_banks->begin(); it != p_banks->end(); it++){
        while( !(*it)->write_back_queue->empty() ){
            Tile* front_tile =  (*it)->write_back_queue->front();
            float bw_used_ = front_tile->write_to_memory(this->bandwidth-bw_used);

            bw_used += bw_used_;

            if (bw_used >= this->bandwidth){
                return;
            }
        }
    }

    if (this->load_queue->empty()) return;
    while (bw_used < this->bandwidth){
        pair<int,Tile*> front = this->load_queue->front();
        while (front.second->is_allocated()){

            //this is too slow
            front.second->bank->push_evict_queue(front.first, front.second);

            this->load_queue->pop_front();
            if (this->load_queue->empty()) return;

            front = this->load_queue->front();
        }

        float bw_used_ = front.second->fetch_from_memory(front.first, this->bandwidth-bw_used);
        if (bw_used_ == 0){ //bank is full, continue with processing to free the banks
            return;
        }
        bw_used += bw_used_;
    }
} 


// void Dram::store(int r){
//     if (this->store_queue->empty()) return;

//     pair<int,Tile*> front = this->store_queue->front();

//     while (front.first <= r){
//         if(!front.second->allocate_on_sram(front.first)){
//             return;
//         }

//         this->store_queue->pop_front();
//         front = this->store_queue->front();
//     }
// }