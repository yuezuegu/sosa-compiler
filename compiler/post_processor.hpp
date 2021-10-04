#ifndef POST_PROCESSOR_HPP
#define POST_PROCESSOR_HPP

#include <map>
#include <list>

#include "ops.hpp"

using namespace std;

class AggrOp;

enum class PP_STATE{
    idle,
    processing,
    done
};

class PostProcessor{
    public:
        int id;
        int last_no_round;

        PostProcessor(int id);

        void assign_op(int r, AggrOp* op);
        AggrOp* get_op(int r);
        bool is_schedule_empty(int r);

        PP_STATE state;
        int exec_cnt;
        P_Tile* curr_tile;

        void init_tile_op(int r);
        bool is_tile_op_done(int r);
        bool is_idle();

        void update();
    private:
        map<int, AggrOp*> schedule;
};

class PostProcessors{
    public:
        int no_pps;

        PostProcessors(int no_pps);
        ~PostProcessors();
        
        list<AggrOp*>* get_schedule(int r);
        list<PostProcessor*>* available_pps(int r);
        bool check_pin1_bank_conflict(int r, P_Tile* p_tile);
        bool check_pin2_bank_conflict(int r, P_Tile* p_tile);
        bool check_pout_bank_conflict(int r, P_Tile* p_tile);
        map<PostProcessor*, Bank*>* get_pin1_permute(int r);
        map<PostProcessor*, Bank*>* get_pin2_permute(int r);
        map<PostProcessor*, Bank*>* get_pout_permute(int r);
        
        map<int, PostProcessor*>* pp_map;

        void init_tile_op(int r);
        bool is_tile_op_done(int r);

        void update();
    private:

        
};


#endif /* POST_PROCESSOR_HPP */
