#ifndef UTILITY_HPP_INCLUDED
#define UTILITY_HPP_INCLUDED

#include <type_traits>

namespace multistage_interconnect {
namespace utility {

template <typename T /* , typename = std::enable_if_t<std::is_integral_v<T>> */>
constexpr bool is_pow2(T t) noexcept {
  return (t != 0) && ((t & (t - 1)) == 0);
}

template <typename T /* , typename = std::enable_if_t<std::is_integral_v<T>> */>
constexpr void inverse_permutation(const T *in, T *out, std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) {
    out[in[i]] = i;
  }
}

} // namespace utility
} // namespace multistage_interconnect

#endif