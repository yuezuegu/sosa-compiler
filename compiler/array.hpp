#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <map>
#include <list>
#include <memory>

#include "ops.hpp"

using namespace std;

enum BUF_STATE{
    empty,
    buffering,
    buffered
};

enum ARR_STATE{
    idle,
    processing,
    done
};

class Array{
    public:
        int id;
        int no_rows, no_cols;
        
        int pipeline_cycles;

        BUF_STATE buf_state;
        W_Tile* curr_w_tile;
        W_Tile* next_w_tile;
        int buf_cnt;

        ARR_STATE arr_state;
        X_Tile* x_tile;
        int exec_cnt;

        int last_no_round;

        Array(int id, int no_rows, int no_cols);
        ~Array();

        void assign_op(int r, MultOp* op);
        MultOp* get_op(int r);
        bool is_idle(int r);
        void init_weight_buffering(int r);
        bool is_weight_buffered(int r);
        
        void init_tile_op(int r);

        void update();
    private:
        map<int, MultOp*> schedule;
};

class Arrays{
    public:
        int no_arrays;

        Arrays(int no_arrays, int no_rows, int no_cols);
        ~Arrays();

        list<MultOp*>* get_schedule(int r);
        list<int>* get_exec_cycles(int r);
        map<Bank*, Array*>* get_x_permute(int r);
        map<Bank*, Array*>* get_w_permute(int r);
        map<Bank*, Array*>* get_pout_permute(int r);
        map<Bank*, Array*>* get_pin_permute(int r);
        bool check_x_bank_conflict(int r, X_Tile* x_tile);
        bool check_w_bank_conflict(int r, W_Tile* w_tile);
        bool check_pout_bank_conflict(int r, P_Tile* p_tile);
        bool check_pin_bank_conflict(int r, P_Tile* p_tile);
        list<Array*>* available_arrays(int r);
        bool is_weights_buffered(int r);

        void init_weight_buffering(int r);

        void update();
        map<int, Array*>* array_map;
    private:
    
        
};





#endif /* ARRAY_HPP */
