#ifndef BANK_HPP
#define BANK_HPP




#include <list>
#include <map>

#include "helper.hpp"

using namespace std;

class Tile;

class Bank{
    public:
        int size; //in bytes
        int id;
        data_type type;
        map<Tile*, int> allocated_tiles;

        Bank(int id, data_type type, int size);
        void alloc_tile(Tile* tile);
        int get_next_virt_addr();
    private:
        int next_virt_addr;
};


class Banks{
    public:
        int no_banks;
        list<Bank> x_banks;
        list<Bank> w_banks;
        list<Bank> p_banks;

        Banks(int no_banks, int size);
        Bank* get_bank_by_id(int id, data_type type);
    private:
};

#endif