
#include "ops.hpp"
#include "array.hpp"


// Op::Op(string layer_name){
//     this->layer_name = layer_name;

//     this->pout_tile = new P_Tile();
// }

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

void MultOp::assign_to_array(int r, Array* array){
    this->array_placed = array;
    this->round_placed = r;
    this->is_placed_ = true;
};

bool Op::is_placed(){
    return this->is_placed_;
}


AggrOp::AggrOp(string layer_name, Op* operand1, Op* operand2, bool flip){
    this->layer_name = layer_name;
    this->operand1 = operand1;
    this->operand2 = operand2;
    this->pin1_tile = operand1->pout_tile;
    this->pin2_tile = operand2->pout_tile;
    this->flip = flip;
    this->pp_placed = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;

    assert((this->pin1_tile->dims == this->pin2_tile->dims && "Input dimensions do not match!"));
    this->pout_tile = new P_Tile(layer_name, make_tuple(-1, -1, -1), this->pin1_tile->dims);
};

Op* AggrOp::get_op1(){
    if (this->flip){
        return this->operand2;
    }
    else{
        return this->operand1;
    }
}

Op* AggrOp::get_op2(){
    if (this->flip){
        return this->operand1;
    }
    else{
        return this->operand2;
    }
}

void AggrOp::assign_to_pp(int r, PostProcessor* pp){
    this->pp_placed = pp;
    this->round_placed = r;
    this->is_placed_ = true;
};


int AggrOp::max_round(){
    AggrOp* op1 = (AggrOp*)this->operand1;
    AggrOp* op2 = (AggrOp*)this->operand2;

    int r1 = op1->round_placed;
    int r2 = op2->round_placed;

    return (r1>r2) ? r1 : r2;    
}




