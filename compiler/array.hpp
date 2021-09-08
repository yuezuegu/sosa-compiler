#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <map>
#include <list>

#include "ops.hpp"

using namespace std;

class Array{
    public:
        int id;
        int no_rows, no_cols;
        
        int last_no_round;

        Array(int id, int no_rows, int no_cols);
        void assign_op(int r, MultOp* op);
        MultOp* get_op(int r);
        bool is_idle(int r);
    private:
        map<int, MultOp*> schedule;
};

class Arrays{
    public:
        int no_arrays;

        Arrays(int no_arrays, int no_rows, int no_cols);
        
        list<MultOp*> get_schedule(int r);
        map<Bank*, Array*> get_x_permute(int r);
        map<Bank*, Array*> get_w_permute(int r);
        map<Bank*, Array*> get_pout_permute(int r);
        map<Bank*, Array*> get_pin_permute(int r);
        bool check_x_bank_conflict(int r, X_Tile* x_tile);
        bool check_w_bank_conflict(int r, W_Tile* w_tile);
        bool check_pout_bank_conflict(int r, P_Tile* p_tile);
        bool check_pin_bank_conflict(int r, P_Tile* p_tile);
        list<Array*> available_arrays(int r);

        map<int, Array*> array_map;
    private:
    
        
};





#endif /* ARRAY_HPP */
