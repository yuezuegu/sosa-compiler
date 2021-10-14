#ifndef DRAM_HPP
#define DRAM_HPP

#include <queue>

#include "tiles.hpp"

using namespace std;

class Dram{
    public:
        float bandwidth;
        queue<Tile*>* memory_queue;

        Dram(float bandwidth);
        ~Dram();

        void update();


    private:

};


#endif