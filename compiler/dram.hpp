#ifndef DRAM_HPP
#define DRAM_HPP

#include <queue>

#include "tiles.hpp"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

class Dram{
    public:
        float bandwidth;
        queue<Tile*>* memory_queue;

        Dram(){};
        Dram(float bandwidth);
        ~Dram(){};

        void update();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->bandwidth;
            ar & this->memory_queue;
        }

    private:

};


#endif