#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <map>
#include <list>
#include <memory>

#include "ops.hpp"
#include "interconnect.hpp"
#include "bank.hpp"

#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

enum class BUF_STATE{
    empty,
    buffering,
    buffered
};

enum class ARR_STATE{
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
        MultOp* curr_op;
        int buf_cnt;

        ARR_STATE arr_state;
        X_Tile* x_tile;
        P_Tile* pin_tile;
        P_Tile* pout_tile;
        int exec_cnt;

        long no_macs;
        long sram_read_bytes;
        long sram_write_bytes;
        long last_no_round;

        Array(){};
        Array(int id, int no_rows, int no_cols);
        ~Array();

        void assign_op(int r, MultOp* op);
        MultOp* get_op(int r);
        bool is_schedule_empty(int r);
        void init_weight_buffering(int r);
        bool is_weight_buffered(int r);
        
        void init_tile_op(int r);
        bool is_tile_op_done(int r);
        bool is_idle();

        void update();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->id;
            ar & this->no_rows;
            ar & this->no_cols;
            ar & this->pipeline_cycles;
            ar & this->last_no_round;
            ar & this->schedule;
            ar & this->buf_state;
            ar & this->curr_w_tile;
            ar & this->next_w_tile;
            ar & this->curr_op;
            ar & this->buf_cnt;
            ar & this->arr_state;
            ar & this->x_tile;
            ar & this->exec_cnt;
        }

    private:
        map<int, MultOp*> schedule;
};

class Arrays{
    public:

        int no_arrays;
        map<int, Array*>* array_map;

        Arrays(){};
        Arrays(int no_arrays, int no_rows, int no_cols);
        ~Arrays();

        list<MultOp*>* get_schedule(int r);
        list<X_Tile*>* get_x_tiles(int r);
        list<W_Tile*>* get_w_tiles(int r);
        list<P_Tile*>* get_pin_tiles(int r);
        list<P_Tile*>* get_pout_tiles(int r);

        map<Array*, Bank*>* get_x_permute(int r);
        map<Array*, Bank*>* get_w_permute(int r);
        map<Array*, Bank*>* get_pout_permute(int r);
        map<Array*, Bank*>* get_pin_permute(int r);
        bool check_x_bank_conflict(int r, X_Tile* x_tile);
        bool check_w_bank_conflict(int r, W_Tile* w_tile);
        bool check_pout_bank_conflict(int r, P_Tile* p_tile);
        bool check_pin_bank_conflict(int r, P_Tile* p_tile);
        list<Array*>* available_arrays(int r);
        bool is_weights_buffered(int r);

        void init_weight_buffering(int r);

        void init_tile_op(int r);
        bool is_tile_op_done(int r);
        bool is_idle();

        void update();

        long total_no_ops();
        long total_sram_read_bytes();
        long total_sram_write_bytes();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->no_arrays;
            ar & this->array_map;
        }

    private:
    
        
};





#endif /* ARRAY_HPP */
