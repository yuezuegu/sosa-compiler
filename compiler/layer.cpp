#include "layer.hpp"

Layer::Layer(string layer_name, 
                tile_dim_map x_tile_dims, 
                tile_dim_map w_tile_dims, 
                tuple<int, int, int> no_tiles, 
                tuple<int, int> input_size, 
                tuple<int, int> weight_size){
    this->layer_name = layer_name;
    this->x_tile_dims = x_tile_dims;
    this->w_tile_dims = w_tile_dims;
    this->no_tiles = no_tiles;
    this->input_size = input_size;
    this->weight_size = weight_size;
}

