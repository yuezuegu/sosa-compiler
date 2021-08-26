
#include "ops.hpp"
#include "array.hpp"


MultOp::MultOp(string layer_name, tuple<int, int, int> op_ind, X_Tile* x_tile, W_Tile* w_tile, P_Tile* pout_tile){
    this->layer_name = layer_name;
    this->op_ind = op_ind;
    this->x_tile = x_tile;
    this->w_tile = w_tile;
    this->pout_tile = pout_tile;
    this->pin_op = nullptr;
    this->array_placed = nullptr;
    this->aggregated_to = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;

}

void MultOp::assign_pin(MultOp* pin_op){
    this->pin_op = pin_op;
    pin_op->aggregated_to = this;
};

void Op::assign_to_array(int r, Array* array){
    this->array_placed = array;
    this->round_placed = r;
    this->is_placed_ = true;
};

bool Op::is_placed(){
    return this->is_placed_;
}


AggrOp::AggrOp(string layer_name, Op* operand1, Op* operand2){
    this->layer_name = layer_name;
    this->operand1 = operand1;
    this->operand2 = operand2;
    this->pin1_tile = operand1->pout_tile;
    this->pin2_tile = operand2->pout_tile;

    assert((this->pin1_tile->dims == this->pin2_tile->dims && "Input dimensions do not match!"));
    this->pout_tile = new P_Tile(layer_name, {-1, -1, -1}, this->pin1_tile->dims);
};






