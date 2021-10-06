
#include "interconnect.hpp"

#include <cmath>

InterconnectBase *generate_interconnect(UnsignedInt n, InterconnectType type) {
#define XYZ(id, class) \
    if ((unsigned) type & (unsigned) InterconnectType :: id) \
        return new class (n);
    
    XYZ(benes_copy, BenesWithCopy)
    XYZ(crossbar, Crossbar)
    XYZ(benes_vanilla, Benes)
#undef XYZ
    if ((unsigned) type & (unsigned) InterconnectType::banyan) {
        bool expansion = (unsigned) type & 0x0Fu;
        auto banyan = new Banyan(n + expansion);
        banyan->set_expansion(expansion);
        return banyan;
    }

    // TODO add crossbar here
    return nullptr;
}

Interconnects::Interconnects(int N, InterconnectType interconnect_type) {
    // TODO find a more efficient algo for this
    int n = std::ceil(std::log2(N));

    x_interconnect = generate_interconnect(n, interconnect_type);
    w_interconnect = generate_interconnect(n, interconnect_type);
    pin_interconnect = generate_interconnect(n, interconnect_type);
    pout_interconnect = generate_interconnect(n, interconnect_type);
    pp_in1_interconnect = generate_interconnect(n, interconnect_type);
    pp_in2_interconnect = generate_interconnect(n, interconnect_type);
    pp_out_interconnect = generate_interconnect(n, interconnect_type);
}

float Interconnects::tdp(int switch_width){
    float tdp = 0;
    tdp += this->x_interconnect->power(switch_width);
    tdp += this->w_interconnect->power(switch_width);
    tdp += this->pin_interconnect->power(switch_width);
    tdp += this->pout_interconnect->power(switch_width);
    tdp += this->pp_in1_interconnect->power(switch_width);
    tdp += this->pp_in2_interconnect->power(switch_width);
    tdp += this->pp_out_interconnect->power(switch_width);
    return tdp;
}




std::istream &operator>>(std::istream &in, InterconnectType &interconnect_type) {
    std::string token;
    in >> token;
#define XYZ(x) else if (token == #x) interconnect_type = InterconnectType :: x;
    if (false) ;
    XYZ(benes_copy)
    XYZ(benes_vanilla)
    XYZ(banyan)
    XYZ(banyan_exp_0)
    XYZ(banyan_exp_1)
    XYZ(banyan_exp_2)
    XYZ(banyan_exp_3)
    XYZ(banyan_exp_4)
    XYZ(crossbar)
    else {
        in.setstate(std::ios_base::failbit);
    }
#undef XYZ
    return in;
}

std::ostream &operator<<(std::ostream &out, InterconnectType interconnect_type) {
    const char *arr[] = {
        "",
        "crossbar",
        "benes_copy",
        "benes_vanilla",
        "banyan_exp_"
    };
    out << arr[(unsigned) interconnect_type >> 4];
    if ((unsigned) interconnect_type & (unsigned) InterconnectType::banyan) {
        out << ((unsigned) interconnect_type & 0x0Fu);
    }
    return out;
}
