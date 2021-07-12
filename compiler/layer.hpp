#include <iostream>
#include <map>
#include <tuple>

using namespace std;

typedef map<tuple<int, int>, tuple<int, int>> tile_dim_map;

class Layer {
    public:
        string layer_name;
        tile_dim_map x_tile_dims;
        tile_dim_map w_tile_dims;
        tuple<int, int, int> no_tiles;
        tuple<int, int> input_size;
        tuple<int, int> weight_size;
        
        Layer(string, tile_dim_map, tile_dim_map, tuple<int, int, int>, tuple<int, int>, tuple<int, int>);
};