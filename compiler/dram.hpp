#ifndef DRAM_HPP
#define DRAM_HPP

#include <queue>

#include "tiles.hpp"
#include "bank.hpp"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/queue.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

class Dram{
    public:
        int prefetch_limit;

        float bandwidth;

        float x_tiles_bw_usage;
        float w_tiles_bw_usage;
        float p_tiles_bw_usage;

        list<pair<int, Tile*>>* load_queue;

        Dram(){};
        Dram(float bandwidth, int prefetch_limit);
        ~Dram(){};

        void update(list<Bank*>* p_banks, int r);

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->bandwidth;
            ar & this->load_queue;
        }

    private:

};


#endif