#ifndef OPS_HPP
#define OPS_HPP


#include <string>
#include <tuple>
#include <cassert>

#include "tiles.hpp"
#include "post_processor.hpp"

using namespace std;

class Array;
class PostProcessor;

class Op{
    public:
        // Op(string layer_name);

        string layer_name;
        
        int round_placed;

        P_Tile* pout_tile;

        bool is_placed();
    protected:
        bool is_placed_;
};

class MultOp: public Op{
    public:
        tuple<int, int, int> op_ind;
        X_Tile* x_tile;
        W_Tile* w_tile;
        // P_Tile* pout_tile;
        MultOp* pin_op;
        MultOp* aggregated_to;
        Array* array_placed;

        MultOp(string layer_name, tuple<int, int, int> op_ind, X_Tile* x_tile, W_Tile* w_tile, P_Tile* pout_tile);
        void assign_pin(MultOp* pin_op);
        void assign_to_array(int r, Array* array);
};

class AggrOp: public Op{
    public:
        P_Tile* pin1_tile;
        P_Tile* pin2_tile;
        // P_Tile* pout_tile;
        
        Op* operand1;
        Op* operand2;

        PostProcessor* pp_placed;

        bool flip; //this allows distinguishing same pairs of operations.

        AggrOp(string layer_name, Op* operand1, Op* operand2, bool flip);
        void assign_to_pp(int r, PostProcessor* pp);
        Op* get_op1();
        Op* get_op2();
};

#endif /* OPS_HPP */
