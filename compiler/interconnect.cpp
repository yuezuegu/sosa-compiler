
#include "interconnect.hpp"

Interconnect::Interconnect(int no_arrays){
    this->no_arrays = no_arrays;
}

void Interconnect::apply_permute(map<Bank*, Array*> permute){
    //TODO: Complete this
    return;
};

bool Interconnect::is_route_free(Bank* bank, Array* array){
    //TODO: Complete this
    return true;
};

Interconnects::Interconnects(int no_arrays){
    this->x_interconnect = new Interconnect(no_arrays);
    this->w_interconnect = new Interconnect(no_arrays);
    this->pin_interconnect = new Interconnect(no_arrays);
    this->pout_interconnect = new Interconnect(no_arrays);
    this->pp_in1_interconnect = new Interconnect(no_arrays);
    this->pp_in2_interconnect = new Interconnect(no_arrays);
    this->pp_out_interconnect = new Interconnect(no_arrays);
}