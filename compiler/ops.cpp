
#include "ops.hpp"



MultOp::MultOp(string layer_name, tuple<int, int, int> op_ind, Tile* x_tile, Tile* w_tile){
    this->layer_name = layer_name;
    this->op_ind = op_ind;
    this->x_tile = x_tile;
    this->w_tile = w_tile;
}




