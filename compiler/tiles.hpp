#ifndef TILES_HPP
#define TILES_HPP

#include <string>
#include <tuple>
#include "helper.hpp"

class Bank;

using namespace std;

class Tile{
    public:
        string layer_name;
        data_type type;
        tuple<int, int> id;
        tuple<int, int> dims;
        
        int bank_id;
        int virt_bank_addr;
        int phys_bank_addr;

        Tile(string layer_name, data_type type, tuple<int, int> id, tuple<int, int> dims);
        void assign_bank(Bank* bank);
        bool is_allocated();
        int get_mem_width();
        int get_mem_height();
    private:
        bool is_allocated_;
};

#endif