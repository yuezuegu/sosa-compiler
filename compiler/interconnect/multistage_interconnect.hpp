#ifndef MULTISTAGE_INTERCONNECT_HPP_INCLUDED
#define MULTISTAGE_INTERCONNECT_HPP_INCLUDED

#include "defs.hpp"
#include "exception.hpp"
#include "utility.hpp"
#include <vector>

namespace multistage_interconnect {

struct SwitchState {
  Int outputs[2] = {-1, -1};
  UnsignedInt input_srcs[2] = {0, 0};

  SwitchState &reset() {
    outputs[0] = -1;
    outputs[1] = -1;
    return *this;
  }

  bool is_straight() const { return outputs[0] == 0 && outputs[1] == 1; }

  bool is_cross() const { return outputs[0] == 1 && outputs[1] == 0; }

  bool is_multicasting() const {
    return outputs[0] == outputs[1] && outputs[0] != -1;
  }

  SwitchState &make_straight() {
    outputs[0] = 0;
    outputs[1] = 1;
    return *this;
  }

  SwitchState &make_cross() {
    outputs[0] = 1;
    outputs[1] = 0;
    return *this;
  }

  SwitchState &make_upperbroadcast() {
    outputs[0] = 0;
    outputs[1] = 0;
    return *this;
  }

  SwitchState &make_lowerbroadcast() {
    outputs[0] = 1;
    outputs[1] = 1;
    return *this;
  }

  bool check_path(UnsignedInt src_id, Int src_port, Int dest_port) const {
    if (src_id == (UnsignedInt)(-1)) // to disable the check temporarily
      return true;

    // TODO revert by commenting the next line
    // return outputs[dest_port] == -1 || input_srcs[outputs[dest_port]] ==
    // src_id;
    return outputs[dest_port] == -1 ||
           (outputs[dest_port] == src_port && input_srcs[src_port] == src_id);
  }

  SwitchState &set_path(UnsignedInt src_id, Int src_port, Int dest_port) {
    input_srcs[src_port] = src_id;
    outputs[dest_port] = src_port;
    return *this;
  }

  Int propagate(UnsignedInt x) const {
    if (x % 2 == 0 && outputs[1] == 0) {
      return x + 1;
    } else if (x % 2 == 1 && outputs[0] == 1) {
      return x - 1;
    } else if (x % 2 == 0 && outputs[0] == 0) {
      return x;
    } else if (x % 2 == 1 && outputs[1] == 1) {
      return x;
    }
    return -1;
  }
};

struct FixedPermutation {
  // mapping from input to outputs
  // mapping[input] = output, which output should the input must be connected
  // to?
  std::vector<UnsignedInt> mapping;

  explicit FixedPermutation(UnsignedInt N = 0) : mapping(N, 0) {}

  template <typename T>
  std::vector<T> propagate(std::vector<T> const &input) const {
#ifndef MIN_NO_SIMPLE_CHECKS
    MIN_REQUIRE(input.size() == mapping.size())
    MIN_REQUIRE(utility::is_pow2(input.size()))
#endif

    const std::size_t N = input.size();
    std::vector<T> result(N);
    for (std::size_t i = 0; i < N; ++i) {
      result[mapping[i]] = input[i];
    }

    return result;
  }

  [[nodiscard]] FixedPermutation inversed() const {
    FixedPermutation result(mapping.size());
    utility::inverse_permutation(&mapping[0], &result.mapping[0],
                                 mapping.size());
    return result;
  }

  FixedPermutation &inverse() { return (*this = inversed()); }
};

struct Stage {
  std::vector<SwitchState> switch_states;

  explicit Stage(std::size_t N = 0,
                 const SwitchState &default_state = SwitchState{})
      : switch_states(N >> 1, default_state) {}

  Stage &reset(const SwitchState &default_state = SwitchState{}) {
    for (auto &sw : switch_states)
      sw = default_state;
    return *this;
  }

  template <typename T>
  [[nodiscard]] std::vector<T> propagate(std::vector<T> const &input,
                                         T invalid = T(-1)) const {
#ifndef MIN_NO_SIMPLE_CHECKS
    MIN_REQUIRE(input.size() == (switch_states.size() << 1))
    MIN_REQUIRE(utility::is_pow2(input.size()))
#endif

    const std::size_t N = input.size();
    std::vector<T> result(N);
    for (std::size_t i = 0; i < (N >> 1); ++i) {
      if (switch_states[i].outputs[0] == -1) {
        result[(i << 1)] = invalid;
      } else {
        result[(i << 1)] = input[(i << 1) + switch_states[i].outputs[0]];
      }

      if (switch_states[i].outputs[1] == -1) {
        result[(i << 1) + 1] = invalid;
      } else {
        result[(i << 1) + 1] = input[(i << 1) + switch_states[i].outputs[1]];
      }
    }

    return result;
  }
};

} // namespace multistage_interconnect

#endif // MULTISTAGE_INTERCONNECT_HPP_INCLUDED
