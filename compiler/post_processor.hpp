#ifndef POST_PROCESSOR_HPP
#define POST_PROCESSOR_HPP

#include <map>
#include <list>

#include "ops.hpp"

using namespace std;

class AggrOp;

class PostProcessor{
    public:
        int id;
        int last_no_round;

        PostProcessor(int id);

        void assign_op(int r, AggrOp* op);
        AggrOp* get_op(int r);
        bool is_idle(int r);

    private:
        map<int, AggrOp*> schedule;
};

class PostProcessors{
    public:
        int no_pps;

        PostProcessors(int no_pps);

        list<AggrOp*> get_schedule(int r);
        list<PostProcessor*> available_pps(int r);
        bool check_pin1_bank_conflict(int r, P_Tile* p_tile);
        bool check_pin2_bank_conflict(int r, P_Tile* p_tile);
        bool check_pout_bank_conflict(int r, P_Tile* p_tile);
        map<Bank*, PostProcessor*> get_pin1_permute(int r);
        map<Bank*, PostProcessor*> get_pin2_permute(int r);
        map<Bank*, PostProcessor*> get_pout_permute(int r);
        
        map<int, PostProcessor*> pp_map;

    private:

        
};


#endif /* POST_PROCESSOR_HPP */
