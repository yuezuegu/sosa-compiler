#ifndef BANK_HPP
#define BANK_HPP

#include <list>
#include <vector>
#include <map>

#include "helper.hpp"

using namespace std;

class Tile;

class Bank{
    public:
        int id;
        data_type type;
        map<Tile*, int> allocated_tiles;
        int capacity; //in terms of bytes

        Bank(int id, data_type type, int capacity);
        ~Bank();
        
        bool alloc_tile(Tile* tile);
        void free_tile(Tile* tile);
    private:
        int capacity_used;
};


class Banks{
    public:
        int no_banks;

        Banks(int no_banks, int bank_size);
        ~Banks();

        Bank* get_bank_by_id(int id, data_type type);
        vector<Bank*> get_x_banks();
        vector<Bank*> get_w_banks();
        vector<Bank*> get_p_banks();
    private:
        vector<Bank*> x_banks{};
        vector<Bank*> w_banks{};
        vector<Bank*> p_banks{};

};

#endif /* BANK_HPP */
