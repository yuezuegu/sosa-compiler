#ifndef DUMP_SCHEDULE_HPP_INCLUDED
#define DUMP_SCHEDULE_HPP_INCLUDED

#include <ostream>
#include <sstream>
#include "compiler.hpp"

inline void dump_schedule(std::ostream &os, Compiler *c) {
    struct pretty_printer {
        pretty_printer(std::ostream &os): os_{os} {
        }

        std::ostream &operator()() {
            for (unsigned i = 0; i < indent_; ++i) {
                os_ << " ";
            }
            return os_;
        }

        void indent() {
            indent_ += 4;
        }

        void outdent() {
            if (indent_ >= 4)
                indent_ -= 4;
        }
    private:
        unsigned indent_ = 0;
        std::ostream &os_;
    };

    pretty_printer p{os};
    
    p() << "{\n";
    p.indent();

    for (unsigned r = 0; r < c->no_main_rounds(); ++r) {
        p() << "{\n";
        p.indent();

        p() << "'round_idx': " << r << "\n";
        p() << "'pattern': [\n";
        p.indent();

        auto mult_ops = c->arrays->get_schedule(r);
        std::vector<MultOp *> pattern(c->arrays->no_arrays, nullptr);

        for (auto mult_op: *mult_ops) {
            if (mult_op == nullptr)
                continue ;
            pattern[mult_op->array_placed->id] = mult_op;
        }

        for (auto op: pattern) {
            if (op == nullptr) {
                p() << "\"" << "none" << "\",\n";
                continue ;
            }
            std::ostringstream oss;
            oss << op->layer_name <<
                "\tind: " <<  get<0>(op->op_ind) << "-" << get<1>(op->op_ind) << "-" << get<2>(op->op_ind);
            p() << "\"" << oss.str() << "\",\n";
        }

        delete mult_ops;

        p.outdent();
        p() << "]\n";

        p.outdent();
        p() << "},\n";
    }

    p.outdent();
    p() << "}\n";
}


#endif // DUMP_SCHEDULE_HPP_INCLUDED
