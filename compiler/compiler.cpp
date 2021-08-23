

#include "compiler.hpp"


using namespace std;


Compiler::Compiler(Banks* banks){
    this->banks = banks;
}

void Compiler::compile(Layers* layers){
    for(auto it = layers->begin(); it != layers->end(); it++){
        compile_layer(&(*it));
    }

}

void Compiler::compile_layer(Layer* layer){
    layer->create_main_ops();
    layer->init_banks(this->banks);

}