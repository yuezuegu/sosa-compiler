#ifndef OPS_HPP
#define OPS_HPP


#include <string>
#include <tuple>

#include "tiles.hpp"

using namespace std;

class MultOp{
    public:
        string layer_name;
        tuple<int, int, int> op_ind;
        Tile* x_tile;
        Tile* w_tile;

        MultOp(string layer_name, tuple<int, int, int> op_ind, Tile* x_tile, Tile* w_tile);
    private:
};

#endif