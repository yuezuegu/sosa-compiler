#ifndef COMPILER_HPP
#define COMPILER_HPP


#include <list>

#include "layer.hpp"

using namespace std;

class Compiler{
    public:
        Banks* banks;

        Compiler(Banks* banks);
        void compile(Layers* layers);
        void compile_layer(Layer* layer);
    private:


};

#endif
