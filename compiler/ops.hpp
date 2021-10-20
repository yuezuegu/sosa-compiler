#ifndef OPS_HPP
#define OPS_HPP


#include <string>
#include <tuple>
#include <cassert>

#include "tiles.hpp"
#include "post_processor.hpp"
#include "serialize_tuple.h"

#include <boost/serialization/map.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

class Array;
class PostProcessor;

class Op{
    public:
        string layer_name;
        
        Op(){};

        int round_placed;
        tuple<int, int, int> op_ind;

        bool is_multop;

        P_Tile* pout_tile;
        Op* pair_op;

        bool retired;
        
        bool is_placed();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->layer_name;
            ar & this->round_placed;
            ar & this->op_ind;
            ar & this->is_multop;
            ar & this->pout_tile;
            ar & this->pair_op;
            ar & this->retired;
            ar & this->is_placed_;
        }

    protected:
        bool is_placed_;
};

class MultOp: public Op{
    public:
        X_Tile* x_tile;
        W_Tile* w_tile;

        MultOp* pin_op;
        MultOp* aggregated_to;
        Array* array_placed;

        int weight_buffer_cycles;

        MultOp(){};
        MultOp(string layer_name, tuple<int, int, int> op_ind, X_Tile* x_tile, W_Tile* w_tile, P_Tile* pout_tile);
        MultOp(const MultOp&);

        void assign_pin(MultOp* pin_op);
        void assign_to_array(int r, Array* array);

        void retire();

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->weight_buffer_cycles;
            ar & this->x_tile;
            ar & this->w_tile;
            ar & this->pin_op;
            ar & this->aggregated_to;
            ar & this->array_placed;  
            ar & this->layer_name;
            ar & this->round_placed;
            ar & this->op_ind;
            ar & this->is_multop;
            ar & this->pout_tile;
            ar & this->pair_op;
            ar & this->retired;
            ar & this->is_placed_;          
        }
};

class AggrOp: public Op{
    public:
        
        P_Tile* pin1_tile;
        P_Tile* pin2_tile;

        Op* operand1;
        Op* operand2;

        int max_round();

        PostProcessor* pp_placed;

        bool flip; //this allows distinguishing same pairs of operations.

        AggrOp(){};
        AggrOp(string layer_name, tuple<int, int, int> op_ind, Op* operand1, Op* operand2, P_Tile* pout_tile, bool flip);
        AggrOp(const AggrOp&) = default;

        void assign_to_pp(int r, PostProcessor* pp);
        Op* get_op1();
        Op* get_op2();
        void set_pair(AggrOp* pair);

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
            ar & this->pin1_tile;
            ar & this->pin2_tile;
            ar & this->operand1;
            ar & this->operand2;
            ar & this->flip;
            ar & this->pp_placed;  
            ar & this->layer_name;
            ar & this->round_placed;
            ar & this->op_ind;
            ar & this->is_multop;
            ar & this->pout_tile;
            ar & this->pair_op;
            ar & this->retired;
            ar & this->is_placed_;          
        }
};

#endif /* OPS_HPP */
