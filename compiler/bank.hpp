#ifndef BANK_HPP
#define BANK_HPP

#include <list>
#include <map>
#include <iostream>
#include <numeric>

#include "helper.hpp"
#include "ops.hpp"

#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

class Tile;

class Bank{
    public:
        int id;
        data_type type;
        map<Tile*, int> allocated_tiles;
        int capacity; //in terms of bytes
        int capacity_used;

        list<pair<int, Tile*>>* evict_queue;
        list<pair<int, Tile*>>* spawn_queue;
        list<Tile*>* write_back_queue;

        Bank(){};
        Bank(int id, data_type type, int capacity);
        ~Bank();
        
        void spawn(int r);
        bool alloc_tile(int r, Tile* tile);
        bool evict(int r);
        void push_evict_queue(int r, Tile* tile);
        void garbage_collect(int r);
        void free_tile(Tile* tile);
        int evict_queue_size();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->id;   
            ar & this->type; 
            ar & this->allocated_tiles; 
            ar & this->capacity;
            ar & this->capacity_used;
        }

    private:
        
};


class Banks{
    public:
        int no_banks;

        Banks(){};
        Banks(int no_banks, int bank_size);
        ~Banks();

        Bank* get_bank_by_id(int id, data_type type);
        list<Bank*>* get_x_banks();
        list<Bank*>* get_w_banks();
        list<Bank*>* get_p_banks();

        void garbage_collect(int r);
        void spawn(int r);

        bool is_write_back_empty();

        void print_usage();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->no_banks;   
            ar & this->x_banks; 
            ar & this->w_banks; 
            ar & this->p_banks;
        }
    private:
        list<Bank*>* x_banks;
        list<Bank*>* w_banks;
        list<Bank*>* p_banks;

};

#endif /* BANK_HPP */
