
#include "ops.hpp"
#include "array.hpp"


MultOp::MultOp(string layer_name, tuple<int, int, int> op_ind, X_Tile* x_tile, W_Tile* w_tile, P_Tile* pout_tile){
    this->layer_name = layer_name;
    this->op_ind = op_ind;
    this->x_tile = x_tile;
    this->x_tile->input_of->push_back(this);
    this->w_tile = w_tile;
    this->w_tile->input_of->push_back(this);
    this->pout_tile = pout_tile;
    this->pout_tile->output_of = this;

    this->pin_op = nullptr;
    this->array_placed = nullptr;
    this->aggregated_to = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;
    this->pair_op = nullptr;

    this->weight_buffer_cycles = get<0>(w_tile->dims);
    this->retired = false;
    this->is_multop = true;
    this->is_finalop = false;
}

MultOp::MultOp(const MultOp& mult_op){
    this->layer_name = mult_op.layer_name;
    this->op_ind = mult_op.op_ind;
    this->x_tile = new X_Tile(*(mult_op.x_tile));
    this->x_tile->input_of->push_back(this);
    this->w_tile = new W_Tile(*(mult_op.w_tile));
    this->w_tile->input_of->push_back(this);
    this->pout_tile = new P_Tile(*(mult_op.pout_tile));
    this->pin_op = nullptr;
    this->array_placed = nullptr;
    this->aggregated_to = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;
    this->pair_op = nullptr;

    this->weight_buffer_cycles = get<0>(this->w_tile->dims);
    this->retired = false;
    this->is_multop = true;
}

void MultOp::retire(){
    this->retired = true;
}

void MultOp::assign_pin(MultOp* pin_op){
    this->pin_op = pin_op;
    this->pin_op->pout_tile->input_of->push_back(this);
    pin_op->aggregated_to = this;
}

void MultOp::assign_to_array(int r, Array* array){
    this->array_placed = array;
    this->round_placed = r;
    this->is_placed_ = true;
}

bool Op::is_placed(){
    return this->is_placed_;
}

AggrOp::AggrOp(string layer_name, tuple<int, int, int> op_ind, Op* operand1, Op* operand2, P_Tile* pout_tile, bool flip){
    this->layer_name = layer_name;
    this->op_ind = op_ind;
    this->operand1 = operand1;
    this->operand2 = operand2;
    this->pin1_tile = operand1->pout_tile;
    this->pin2_tile = operand2->pout_tile;
    this->flip = flip;
    this->pp_placed = nullptr;
    this->round_placed = -1;
    this->is_placed_ = false;
    this->pair_op = nullptr;
    this->is_multop = false;
    this->retired = false;
    this->is_finalop = false;

    assert((this->pin1_tile->dims == this->pin2_tile->dims && "Input dimensions do not match!"));
    if (pout_tile == nullptr){
        auto dims = this->pin1_tile->dims;
        int memory_size = get<0>(dims) * get<1>(dims) * 2;
        this->pout_tile = new P_Tile(layer_name, make_tuple(-1, -1, -1), dims, 2, memory_size);
    }
    else{
        this->pout_tile = pout_tile;
    }

    this->pout_tile->output_of = this;
}

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

void AggrOp::retire(){
    this->retired = true;
}

void AggrOp::assign_to_pp(int r, PostProcessor* pp){
    this->pp_placed = pp;
    this->round_placed = r;
    this->is_placed_ = true;
}

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




