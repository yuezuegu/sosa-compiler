#ifndef TILES_HPP
#define TILES_HPP

#include <string>
#include <tuple>
#include <list>

#include "helper.hpp"

class Bank;
class Op;

using namespace std;

class Tile{
    public:
        string layer_name;
        data_type type;

        tuple<int, int> dims;
        int precision; //In terms of number of bytes
        
        int bytes_fetched_from_memory; //In terms of number of bytes
        int memory_size; //In terms of number of bytes

        list<Op*>* input_of;

        int bank_id;
        int virt_bank_addr;
        int phys_bank_addr;

        Bank* bank;

        void assign_bank(Bank* bank);
        bool is_allocated();
        int get_mem_width();
        int get_mem_height();
        void fetch_from_memory(int bytes);
        void try_free();

    protected:
        bool is_allocated_on_sram;
};

class X_Tile: public Tile{
    public:
        tuple<int, int> id;
        
        X_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision = 1);
        X_Tile(const X_Tile& x_tile) = default;
};

class W_Tile: public Tile{
    public:
        tuple<int, int> id;

        W_Tile(string layer_name, tuple<int, int> id, tuple<int, int> dims, int precision = 1);
        W_Tile(const W_Tile& w_tile) = default;
};

class P_Tile: public Tile{
    public:
        tuple<int, int, int> id;
        
        P_Tile(string layer_name, tuple<int, int, int> id, tuple<int, int> dims, int precision = 2);
        P_Tile(const P_Tile& p_tile) = default;
};


#endif /* TILES_HPP */
