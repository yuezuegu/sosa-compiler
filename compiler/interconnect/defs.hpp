#ifndef DEFS_HPP_INCLUDED
#define DEFS_HPP_INCLUDED

#if __cplusplus < 201703L
// #error "This program requires a C++17-compatible compiler to be run."
#endif

// Some definitions:
//  * a mapping maps an input to an output
//    mapping[input] = output
//  * an inverse mapping maps an output to an input
//    inverse_mapping[output] = input
//    an input can be multicast to multiple destination ports

// uncomment the following line to disable longer runtime checks
// #define MIN_NO_LONG_CHECKS

// uncomment the following line to disable simple checks
// #define MIN_NO_SIMPLE_CHECKS

namespace multistage_interconnect {

using Int = int;
using UnsignedInt = unsigned long;

} // namespace multistage_interconnect

#endif
