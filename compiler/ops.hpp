#ifndef OPS_HPP
#define OPS_HPP


#include <string>
#include <tuple>
#include <cassert>

#include "tiles.hpp"

using namespace std;

class Array;

class Op{
    public:
        string layer_name;
        Array* array_placed;
        int round_placed;

        P_Tile* pout_tile;

        void assign_to_array(int r, Array* array);
        bool is_placed();

    protected:
        bool is_placed_;
};

class MultOp: public Op{
    public:
        tuple<int, int, int> op_ind;
        X_Tile* x_tile;
        W_Tile* w_tile;
        P_Tile* pout_tile;
        MultOp* pin_op;
        MultOp* aggregated_to;
        
        MultOp(string layer_name, tuple<int, int, int> op_ind, X_Tile* x_tile, W_Tile* w_tile, P_Tile* pout_tile);
        void assign_pin(MultOp* pin_op);
};

class AggrOp: public Op{
    public:
        P_Tile* pin1_tile;
        P_Tile* pin2_tile;
        P_Tile* pout_tile;
        
        Op* operand1;
        Op* operand2;

        AggrOp(string layer_name, Op* operand1, Op* operand2);
};

#endif /* OPS_HPP */
