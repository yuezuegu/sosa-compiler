#ifndef TILES_HPP
#define TILES_HPP

#include <string>
#include <tuple>
#include <list>

#include "helper.hpp"
#include "serialize_tuple.h"

#include <boost/log/trivial.hpp>


#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

class Bank;
class Op;

using namespace std;

class Tile{
    public:
        string layer_name;
        data_type type;

        string tag;

        tuple<int, int> dims;
        int precision; //In terms of number of bytes
        
        float bytes_fetched_from_memory; //In terms of number of bytes
        float bytes_written_to_memory; //In terms of number of bytes
        int memory_size; //In terms of number of bytes

        list<Op*>* input_of;
        Op* output_of;

        Bank* bank;

        bool is_spawn_;
        bool is_allocated_on_sram;
        
        void assign_bank(Bank* bank);
        bool is_allocated();
        int get_mem_width();
        int get_mem_height();
        float fetch_from_memory(int, int, float);
        float write_to_memory(float);
        bool allocate_on_sram(int, int);
        void remove_from_sram();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->layer_name;   
            ar & this->type; 
            ar & this->dims; 
            ar & this->precision; 
            ar & this->memory_size; 
            ar & this->bytes_fetched_from_memory; 
            ar & this->input_of;
            ar & this->bank;
            ar & this->is_allocated_on_sram;
        }

        
};

class X_Tile: public Tile{
    public:
        tuple<int, int> id;
        
        X_Tile(){};
        X_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision, int memory_size);
        X_Tile(const X_Tile& x_tile) = default;

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->layer_name;   
            ar & this->type; 
            ar & this->dims; 
            ar & this->precision; 
            ar & this->memory_size; 
            ar & this->bytes_fetched_from_memory; 
            ar & this->input_of;
            ar & this->bank;
            ar & this->is_allocated_on_sram;
            ar & this->id;
        }
};

class W_Tile: public Tile{
    public:
        tuple<int, int> id;

        W_Tile(){};
        W_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision, int memory_size);
        W_Tile(const W_Tile& w_tile) = default;

        friend class boost::serialization::access;
        
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->layer_name;   
            ar & this->type; 
            ar & this->dims; 
            ar & this->precision; 
            ar & this->memory_size; 
            ar & this->bytes_fetched_from_memory; 
            ar & this->input_of;
            ar & this->bank;
            ar & this->is_allocated_on_sram;
            ar & this->id;
        }     
};

class P_Tile: public Tile{
    public:
        tuple<int, int, int> id;
        
        P_Tile(){};
        P_Tile(string layer_name, tuple<int, int, int> id, tuple<int, int> dims, int precision, int memory_size);
        P_Tile(const P_Tile& p_tile) = default;

        friend class boost::serialization::access;
        
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->layer_name;   
            ar & this->type; 
            ar & this->dims; 
            ar & this->precision; 
            ar & this->memory_size; 
            ar & this->bytes_fetched_from_memory; 
            ar & this->input_of;
            ar & this->bank;
            ar & this->is_allocated_on_sram;
            ar & this->id;
        }
};


#endif /* TILES_HPP */
