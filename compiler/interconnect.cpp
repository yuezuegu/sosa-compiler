
#include "interconnect.hpp"

template<class T>
Interconnect<T>::Interconnect(int no_arrays){
    this->no_arrays = no_arrays;
}
template<class T>
void Interconnect<T>::apply_permute(map<Bank*, T*>* permute){
    //TODO: Complete this
    return;
};

template<class T>
bool Interconnect<T>::is_route_free(Bank* bank, T* array){
    //TODO: Complete this
    return true;
};

Interconnects::Interconnects(int no_arrays){
    this->x_interconnect = new Interconnect<Array>(no_arrays);
    this->w_interconnect = new Interconnect<Array>(no_arrays);
    this->pin_interconnect = new Interconnect<Array>(no_arrays);
    this->pout_interconnect = new Interconnect<Array>(no_arrays);
    this->pp_in1_interconnect = new Interconnect<PostProcessor>(no_arrays);
    this->pp_in2_interconnect = new Interconnect<PostProcessor>(no_arrays);
    this->pp_out_interconnect = new Interconnect<PostProcessor>(no_arrays);
}

template class Interconnect<Array>;
template class Interconnect<PostProcessor>;
