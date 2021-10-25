#ifndef HELPER_HPP
#define HELPER_HPP

#include <string>

enum data_type {X, W, P};

#define PRINT_TYPE(T) ( (T)==data_type::X ? ("X") : ( (T)==data_type::W ? ("W") : ("P")) )

#endif