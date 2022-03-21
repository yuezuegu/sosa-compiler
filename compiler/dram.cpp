

#include "dram.hpp"




Dram::Dram(float bandwidth, int prefetch_limit){
    this->bandwidth = bandwidth; //bytes per cycle
    this->prefetch_limit = prefetch_limit;

    this->x_tiles_bw_usage = 0; //bytes
    this->w_tiles_bw_usage = 0;
    this->p_tiles_bw_usage = 0;

    this->load_queue = new list<pair<int, Tile*>>();
}

void Dram::update(list<Bank*>* p_banks, int r){
    float bw_used = 0;

    if (this->load_queue->empty()) return;
    while (bw_used < this->bandwidth){
        pair<int,Tile*> front = this->load_queue->front();
        while (front.second->is_allocated()){

            //this might be too slow, replace list with map
            front.second->bank->push_evict_queue(front.first, front.second);

            this->load_queue->pop_front();
            if (this->load_queue->empty()) return;

            front = this->load_queue->front();
            
        }

        if (front.first - r >= this->prefetch_limit) return;
        float bw_used_ = front.second->fetch_from_memory(r, front.first, this->bandwidth-bw_used);
        if (bw_used_ == 0){ //bank is full, continue with processing to free the banks
            break;
        }

        if (front.second->type == data_type::X){
            this->x_tiles_bw_usage += bw_used_;
        }
        else if (front.second->type == data_type::W){
            this->w_tiles_bw_usage += bw_used_;
        }
        else{
            this->p_tiles_bw_usage += bw_used_;
        }

        bw_used += bw_used_;
    }

    for (auto it = p_banks->begin(); it != p_banks->end(); it++){
        while( !(*it)->write_back_queue->empty() ){
            Tile* front_tile =  (*it)->write_back_queue->front();
            float bw_used_ = front_tile->write_to_memory(this->bandwidth-bw_used);
            this->p_tiles_bw_usage += bw_used_;

            bw_used += bw_used_;

            if (bw_used >= this->bandwidth){
                return;
            }
        }
    }
} 
