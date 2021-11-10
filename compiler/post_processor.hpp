#ifndef POST_PROCESSOR_HPP
#define POST_PROCESSOR_HPP

#include <map>
#include <list>

#include "ops.hpp"

#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

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

        PP_STATE state;
        int exec_cnt;
        P_Tile* pin1_tile;
        P_Tile* pin2_tile;
        P_Tile* pout_tile;

        AggrOp* curr_op;

        PostProcessor(){};
        PostProcessor(int id);

        void assign_op(int r, AggrOp* op);
        AggrOp* get_op(int r);
        bool is_schedule_empty(int r);

        void init_tile_op(int r);
        bool is_tile_op_done(int r);
        bool is_idle();
        


        void update();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->id;
            ar & this->last_no_round;
            ar & this->state;
            ar & this->exec_cnt;
            ar & this->pin1_tile;
            ar & this->pin2_tile;
            ar & this->pout_tile;
            ar & this->schedule;
        }
    private:
        map<int, AggrOp*> schedule;
};

class PostProcessors{
    public:
        int no_pps;
        map<int, PostProcessor*>* pp_map;

        PostProcessors(){};
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

        bool is_pin1_ready(int r);
        bool is_pin2_ready(int r);
        bool is_pout_ready(int r);

        list<P_Tile*>* get_pin1_tiles(int r);
        list<P_Tile*>* get_pin2_tiles(int r);
        list<P_Tile*>* get_pout_tiles(int r);

        void init_tile_op(int r);
        bool is_tile_op_done(int r);

        void update();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->no_pps;
            ar & this->pp_map;
        }
    private:

        
};


#endif /* POST_PROCESSOR_HPP */
