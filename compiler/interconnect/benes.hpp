#ifndef BENES_HPP_INCLUDED
#define BENES_HPP_INCLUDED

#include <algorithm>
#include <chrono>
#include <iostream>
#include "multistage_interconnect.hpp"
#include <random>

namespace multistage_interconnect {

struct Benes {
  std::vector<Stage> stages;
  std::vector<FixedPermutation> interstages;

  explicit Benes(UnsignedInt n = 1) { construct(n); }

  Benes &construct(UnsignedInt n) {
    n_ = n;
    const auto num_stages = 2 * n - 1;
    stages = std::vector<Stage>(num_stages, Stage(1 << n));
    interstages =
        std::vector<FixedPermutation>(num_stages - 1, FixedPermutation(1 << n));

    do_connections_half(n, 0);
    do_connections_full();

    return *this;
  }

  Benes &reset(const SwitchState &default_state = SwitchState{}) {
    for (auto &stage : stages)
      stage.reset(default_state);
    return *this;
  }

  template <typename T>
  [[nodiscard]] std::vector<T> propagate(std::vector<T> const &input,
                                         T invalid = T(-1)) const {
    auto last = input;

    auto it = interstages.begin();
    for (auto const &stage : stages) {
      last = stage.propagate(last, invalid);
      if (it != interstages.end()) {
        last = it->propagate(last);
        ++it;
      }
    }

    return last;
  }

  Benes &looping(std::vector<UnsignedInt> const &permutation) {
#ifndef MIN_NO_SIMPLE_CHECKS
    MIN_REQUIRE(permutation.size() == (1u << n_))
#endif

#ifndef MIN_NO_LONG_CHECKS
    // ensure that we are given a permutation
    {
      std::vector<int> tester(permutation.size(), 0);
      for (auto x : permutation) {
        MIN_REQUIRE_MSG(x < permutation.size(),
                        "Element must be in the correct range!")
        MIN_REQUIRE_MSG(tester[x] == 0, "Element must not be repeated!")
        tester[x] = 1;
      }
    }
#endif

    do_looping(n_, 0, std::move(permutation));

    return *this;
  }

  bool bit_follow_raghavendra(UnsignedInt src_port, UnsignedInt dest_port) {
    UnsignedInt port = src_port;
    UnsignedInt routing_bits = dest_port;

    // algorithm BL
    for (UnsignedInt k = 0; k < n_ - 1; ++k) {
      auto &stage = stages[k];
      auto &sw = stage.switch_states[port >> 1];

      UnsignedInt routing_bit = routing_bits & 0x1;
      routing_bits >>= 1;

      bool path_free = sw.check_path(src_port, port % 2, routing_bit);
      if (!path_free) {
        if (sw.is_multicasting()) {
          // this is a multicasting switch, no path out
          // we need to break it
          return false;
        }
        sw.set_path(src_port, port % 2, 1 - routing_bit);
        port = port - (port % 2) + (1 - routing_bit);
      } else {
        sw.set_path(src_port, port % 2, routing_bit);
        port = port - (port % 2) + routing_bit;
      }
      port = interstages[k].mapping[port];
    }

    routing_bits = dest_port;

    // algorithm Omega
    for (UnsignedInt k = n_ - 1; k < 2 * n_ - 1; ++k) {
      auto &stage = stages[k];
      auto &sw = stage.switch_states[port >> 1];

      UnsignedInt routing_bit = (routing_bits & (1ull << (n_ - 1))) > 0;
      routing_bits <<= 1;

      bool path_free = sw.check_path(src_port, port % 2, routing_bit);
      if (!path_free) {
        return false;
      }
      sw.set_path(src_port, port % 2, routing_bit);
      port = port - (port % 2) + routing_bit;
      if (k < 2 * n_ - 2)
        port = interstages[k].mapping[port];
    }

    return true;
  }

  bool bit_follow_dummy(UnsignedInt src_port, UnsignedInt dest_port) {
    UnsignedInt port = src_port;

    for (UnsignedInt k = 0; k < n_ - 1; ++k) {
      auto &stage = stages[k];
      auto &sw = stage.switch_states[port >> 1];

      sw.set_path(src_port, port % 2, port % 2);
      port = interstages[k].mapping[port];
    }

    UnsignedInt routing_bits = dest_port;

    // algorithm Omega
    for (UnsignedInt k = n_ - 1; k < 2 * n_ - 1; ++k) {
      auto &stage = stages[k];
      auto &sw = stage.switch_states[port >> 1];

      UnsignedInt routing_bit = (routing_bits & (1ull << (n_ - 1))) > 0;
      routing_bits <<= 1;

      bool path_free = sw.check_path(src_port, port % 2, routing_bit);
      if (!path_free) {
        return false;
      }
      sw.set_path(src_port, port % 2, routing_bit);
      port = port - (port % 2) + routing_bit;
      if (k < 2 * n_ - 2)
        port = interstages[k].mapping[port];
    }

    return true;
  }

  bool raghavendra(Int const *inverse_mapping) {
    for (UnsignedInt i = 0; i < (1u << n_); ++i) {
      if (inverse_mapping[i] == -1) {
        continue;
      }

      if (!bit_follow_raghavendra(inverse_mapping[i], i)) {
        return false;
      }
    }
    return true;
  }

  bool dummy(Int const *inverse_mapping) {
    for (UnsignedInt i = 0; i < (1u << n_); ++i) {
      if (inverse_mapping[i] == -1) {
        continue;
      }

      if (!bit_follow_dummy(inverse_mapping[i], i)) {
        return false;
      }
    }
    return true;
  }

  void follow_and_set_inputs_srcs() {
    auto last = std::vector<Int>(1 << n_, 0);
    for (Int i = 0; i < (1 << n_); ++i)
      last[i] = i;

    auto it = interstages.begin();
    for (auto &stage : stages) {
      for (Int i = 0; i < (1 << n_); ++i) {
        if (last[i] >= 0) {
          stage.switch_states[i >> 1].input_srcs[i % 2] = last[i];
        }
      }

      last = stage.propagate(last, -1);

      if (it != interstages.end()) {
        last = it->propagate(last);
        ++it;
      }
    }
  }

  bool bit_follow_from_the_middle(UnsignedInt src_port, UnsignedInt dest_port) {
    UnsignedInt routing_bits = dest_port;
    Int port = src_port;

    for (UnsignedInt k = 0; k < n_ - 1; ++k) {
      port = stages[k].switch_states[port >> 1].propagate(port);
      port = interstages[k].mapping[port];
    }

    for (UnsignedInt k = n_ - 1; k < 2 * n_ - 1; ++k) {
      auto &stage = stages[k];
      auto &sw = stage.switch_states[port >> 1];

      UnsignedInt routing_bit = (routing_bits & (1ull << (n_ - 1))) > 0;
      routing_bits <<= 1;

      bool path_free = sw.check_path(src_port, port % 2, routing_bit);
      if (!path_free) {
        return false;
      }
      sw.set_path(src_port, port % 2, routing_bit);
      port = port - (port % 2) + routing_bit;
      if (k < 2 * n_ - 2)
        port = interstages[k].mapping[port];
    }

    return true;
  }

  bool looping_multicast(Int const *inverse_mapping, Int k = 10) {
    // randomness
    static std::mt19937_64 mt(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());

    const UnsignedInt N = 1 << n_;

    // list of unused source ports
    std::vector<UnsignedInt> unused_sources;

    // to detect which ports are left empty
    std::vector<int> is_port_used(N, false);

    UnsignedInt num_used_ports = 0;

    // for looping algorithm
    std::vector<UnsignedInt> permutation(N, 0);

    for (UnsignedInt i = 0; i < N; ++i) {
      if (inverse_mapping[i] >= 0) {
        if (!is_port_used[inverse_mapping[i]]) {
          is_port_used[inverse_mapping[i]] = true;
          ++num_used_ports;
        }
      }
    }

    // unicasting, return true
    if (num_used_ports == N) {
      for (UnsignedInt i = 0; i < N; ++i) {
        permutation[inverse_mapping[i]] = i;
      }
      looping(permutation);
      return true;
    }

    // make a list of the unusued ports
    for (UnsignedInt i = 0; i < N; ++i) {
      if (!is_port_used[i])
        unused_sources.push_back(i);
    }

    UnsignedInt counter = 0;
    do {
      // early termination
      if (k >= 0 && counter == (UnsignedInt) k)
        break;

      // random shuffle the input ports
      shuffle(unused_sources.begin(), unused_sources.end(), mt);

      // index of the next source port to replace
      UnsignedInt j = 0;

      // keep track of the multicasting source ports
      std::vector<int> is_encountered(N, false);

      // for each output port i
      for (UnsignedInt i = 0; i < N; ++i) {
        if (inverse_mapping[i] == -1 || is_encountered[inverse_mapping[i]]) {
          // if None or multicasting, assign the next unused port
          permutation[unused_sources[j]] = i;
          ++j;
        } else {
          // otherwise, make the usual assignment
          permutation[inverse_mapping[i]] = i;
          is_encountered[inverse_mapping[i]] = true;
        }
      }

      // apply the routing algorithm
      looping(permutation);

      // set the src_id's of the switches
      follow_and_set_inputs_srcs();

      // relax the network by removing the unused links in switches
      for (auto &stage : stages) {
        for (auto &sw : stage.switch_states) {
          if (!is_port_used[sw.input_srcs[sw.outputs[0]]]) {
            sw.outputs[0] = -1;
          }
          if (!is_port_used[sw.input_srcs[sw.outputs[1]]]) {
            sw.outputs[1] = -1;
          }
        }
      }

      // apply the butterfly routing from the middle switch
      is_encountered = std::vector<int>(N, false);

      bool success = true;

      for (Int i = 0; i < (1 << n_); ++i) {
        Int j = inverse_mapping[i];
        if (j >= 0) {
          if (!is_encountered[j]) {
            is_encountered[j] = true;
          } else {
            if (!bit_follow_from_the_middle(j, i)) {
              success = false;
              break;
            }
          }
        }
      }

      if (success) {
        // std::cout << " (trials: " << counter << ") ";
        return true;
      }

      counter++;
    } while (true); // std::next_permutation(unused_sources.begin(),
                    // unused_sources.end()));

    return false;
  }

  UnsignedInt n() const { return n_; }

private:
  UnsignedInt n_;

  void do_connections_half(const UnsignedInt i, const UnsignedInt offset) {
    if (i == 1)
      return;

    {
      auto &pattern = interstages[n_ - i];

      for (UnsignedInt j = 0; j < (1u << (i - 1)); ++j) {
        pattern.mapping[offset + (j << 1)] = offset + j;
        pattern.mapping[offset + (j << 1) + 1] = offset + (1 << (i - 1)) + j;
      }
    }

    do_connections_half(i - 1, offset);
    do_connections_half(i - 1, offset + (1 << (i - 1)));
  }

  void do_connections_full() {
    const auto sz = interstages.size();
    for (UnsignedInt j = 0; j < (sz >> 1); ++j) {
      interstages[sz - j - 1] = interstages[j].inversed();
    }
  }

  void do_looping(const UnsignedInt i, const UnsignedInt offset,
                  std::vector<UnsignedInt> const &permutation) {
    if (i == 1) /* base case */ {
      auto &stage = stages[n_ - i];
      if (permutation[0] == 0) {
        stage.switch_states[offset].make_straight();
      } else {
        stage.switch_states[offset].make_cross();
      }
    } else {
      std::vector<UnsignedInt> inverse(permutation.size());

      {
        UnsignedInt j = 0;
        for (auto x : permutation) {
          inverse[x] = j;
          ++j;
        }
      }

      const auto sz = (1u << i);

      std::vector<UnsignedInt> upper_perm(sz >> 1, 0), lower_perm(sz >> 1, 0);

      std::vector<int> marked(sz >> 1, 0);

      auto &in_stage = stages[n_ - i];
      auto &out_stage = stages[n_ + i - 2];

      for (UnsignedInt j = 0; j < (sz >> 1); ++j) {
        if (marked[j])
          continue;
        // foreach non-marked in switch

        in_stage.switch_states[offset + j].make_straight();

        UnsignedInt current_switch = j;

        do {
          marked[current_switch] = true;

          // Loop forward
          UnsignedInt mapped, mapped_inverse;

          if (in_stage.switch_states[offset + current_switch].is_straight()) {
            mapped = permutation[current_switch << 1];
          } else /* Cross */ {
            mapped = permutation[(current_switch << 1) + 1];
            // lower input must be mapped
          }

          upper_perm[current_switch] = mapped >> 1;

          if (mapped % 2) {
            out_stage.switch_states[offset + (mapped >> 1)].make_cross();
            mapped_inverse = inverse[mapped - 1];
          } else {
            out_stage.switch_states[offset + (mapped >> 1)].make_straight();
            mapped_inverse = inverse[mapped + 1];
          }

          // Loop backward
          if (mapped_inverse % 2) {
            in_stage.switch_states[offset + (mapped_inverse >> 1)]
                .make_straight();
          } else {
            in_stage.switch_states[offset + (mapped_inverse >> 1)].make_cross();
          }

          lower_perm[mapped_inverse >> 1] = mapped >> 1;

          current_switch = mapped_inverse >> 1;
        } while (current_switch != j);
      }

      do_looping(i - 1, offset, upper_perm);
      do_looping(i - 1, offset + (1 << (i - 2)), lower_perm);
    }
  }
};

} // namespace multistage_interconnect

#endif // BENES_HPP_INCLUDED
