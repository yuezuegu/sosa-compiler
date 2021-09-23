
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
    this->pair_op = nullptr;

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


AggrOp::AggrOp(string layer_name, Op* operand1, Op* operand2, P_Tile* pout_tile, bool flip){
    this->layer_name = layer_name;
    this->operand1 = operand1;
    this->operand2 = operand2;
    this->pin1_tile = operand1->pout_tile;
    this->pin2_tile = operand2->pout_tile;
    this->flip = flip;
    this->pp_placed = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;
    this->pair_op = nullptr;

    assert((this->pin1_tile->dims == this->pin2_tile->dims && "Input dimensions do not match!"));
    if (pout_tile == nullptr){
        this->pout_tile = new P_Tile(layer_name, make_tuple(-1, -1, -1), this->pin1_tile->dims);
    }
    else{
        this->pout_tile = pout_tile;
    }
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

void AggrOp::set_pair(AggrOp* pair_op){
    this->pair_op = pair_op;
    pair_op->pair_op = this;
}

int AggrOp::max_round(){
    AggrOp* op1 = (AggrOp*)this->operand1;
    AggrOp* op2 = (AggrOp*)this->operand2;

    int r1 = op1->round_placed;
    int r1_pair = -1;
    if (op1->pair_op != nullptr){
        r1_pair = op1->pair_op->round_placed;
    }
    
    int r2 = op2->round_placed;
    int r2_pair = -1;
    if (op2->pair_op != nullptr){
        r2_pair = op2->pair_op->round_placed;
    }
    
    int max_round = -1;
    if (r1 > max_round) max_round = r1;
    if (r1_pair > max_round) max_round = r1_pair;
    if (r2 > max_round) max_round = r2;
    if (r2_pair > max_round) max_round = r2_pair;

    return max_round;    
}




