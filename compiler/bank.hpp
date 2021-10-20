#ifndef BANK_HPP
#define BANK_HPP

#include <list>
#include <map>

#include "helper.hpp"

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

        Bank(){};
        Bank(int id, data_type type, int capacity);
        ~Bank();
        
        bool alloc_tile(Tile* tile);
        void free_tile(Tile* tile);

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
        int capacity_used;
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
