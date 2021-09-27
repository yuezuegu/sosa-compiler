#ifndef BANYAN_HPP_INCLUDED
#define BANYAN_HPP_INCLUDED

#include <algorithm>
#include "multistage_interconnect.hpp"

namespace multistage_interconnect {

struct Banyan {
  std::vector<Stage> stages;
  std::vector<FixedPermutation> interstages;

  explicit Banyan(UnsignedInt n = 1) { construct(n); }

  Banyan &construct(UnsignedInt n) {
    n_ = n;
    const auto num_stages = n;
    stages = std::vector<Stage>(num_stages, Stage(1 << n));
    interstages =
        std::vector<FixedPermutation>(num_stages + 1, FixedPermutation(1 << n));

    do_connections_first_last_stages();
    do_connections(n, 0);

    return *this;
  }

  Banyan &inverse() {
    std::reverse(interstages.begin(), interstages.end());
    for (auto &interstage : interstages)
      interstage.inverse();
    return *this;
  }

  [[nodiscard]] Banyan inversed() const { return Banyan{*this}.inverse(); }

  Banyan &reset(const SwitchState &default_state = SwitchState{}) {
    for (auto &stage : stages)
      stage.reset(default_state);
    return *this;
  }

  template <typename T>
  [[nodiscard]] std::vector<T> propagate(std::vector<T> const &input,
                                         T invalid = T(-1)) const {
    auto last = input;

    auto it = interstages.begin();
    last = it->propagate(last);
    ++it;
    for (auto const &stage : stages) {
      last = stage.propagate(last, invalid);
      last = it->propagate(last);
      ++it;
    }

    return last;
  }

  bool bit_follow(UnsignedInt src_port, UnsignedInt dest_port,
                  bool msb_to_lsb = true, bool set_path = true) {
    auto last = src_port;
    auto it = interstages.begin();
    last = it->mapping[last];

    UnsignedInt mask;
    if (msb_to_lsb)
      mask = 1u << (n_ - 1);
    else
      mask = 1u;

    for (auto &stage : stages) {
      auto &sw = stage.switch_states[last >> 1];
      UnsignedInt routing_bit = (dest_port & mask) > 0;
      if (!sw.check_path(src_port, last % 2, routing_bit)) {
        return false;
      }
      if (set_path) {
        sw.set_path(src_port, last % 2, routing_bit);
      }
      if (routing_bit == 0) {
        if (last % 2 == 1) {
          last = last - 1;
        }
      } else {
        if (last % 2 == 0) {
          last = last + 1;
        }
      }
      if (msb_to_lsb)
        mask >>= 1;
      else
        mask <<= 1;
      it++;
      last = it->mapping[last];
    }
    return true;
  }

  bool bit_follow_expansion(UnsignedInt src_port, UnsignedInt dest_port,
                            UnsignedInt exp, bool set_path = true) {
    return bit_follow(src_port, dest_port << exp, true, set_path);
  }

  bool bit_follow(Int const *inverse_mapping, bool msb_to_lsb = true) {
    for (UnsignedInt i = 0; i < (1 << n_); ++i) {
      if (inverse_mapping[i] == -1) {
        continue;
      }

      if (!bit_follow(inverse_mapping[i], i, msb_to_lsb, true)) {
        return false;
      }
    }
    return true;
  }

  bool bit_follow_expansion(Int const *inverse_mapping, UnsignedInt exp) {
    for (UnsignedInt i = 0; i < (1 << n_); ++i) {
      if (inverse_mapping[i] == -1) {
        continue;
      }

      if (!bit_follow_expansion(inverse_mapping[i], i, exp, true)) {
        return false;
      }
    }
    return true;
  }

  Banyan &boolean_splitting(
      std::vector<std::pair<UnsignedInt, UnsignedInt>> const &packets) {
#ifndef MIN_NO_LONG_CHECKS
    {
      auto y = packets.begin() + 1;
      for (auto &x : packets) {
        MIN_REQUIRE_MSG(
            x.first <= x.second,
            "for each interval, MIN must be less than or equal to MAX");
        MIN_REQUIRE_MSG(x.second < (1 << n_), "invalid port for given n");

        if (y != packets.end()) {
          MIN_REQUIRE_MSG(x.second < y->first,
                          "array of intervals must be sorted");
          ++y;
        }
      }
    }
#endif

    for (std::size_t i = 0; i < packets.size(); ++i) {
      do_boolean_splitting_send_packet(0, 1u << (n_ - 1), (1u << (n_ - 1)) - 1,
                                       (1u << n_) - 1,
                                       interstages[0].mapping[i], packets[i]);
    }

    return *this;
  }

  Banyan &boolean_splitting(UnsignedInt input,
                            std::pair<UnsignedInt, UnsignedInt> packet) {
    do_boolean_splitting_send_packet(0, 1u << (n_ - 1), (1u << (n_ - 1)) - 1,
                                     (1u << n_) - 1,
                                     interstages[0].mapping[input], packet);

    return *this;
  }

  UnsignedInt n() const { return n_; }

private:
  UnsignedInt n_;

  void do_connections_first_last_stages() {
    auto &first = interstages.front();
    auto &last = interstages.back();

    for (UnsignedInt j = 0; j < (1 << (n_ - 1)); ++j) {
      first.mapping[j] = j << 1;
      first.mapping[j + (1 << (n_ - 1))] = (j << 1) + 1;
      last.mapping[(j << 1)] = (j << 1);
      last.mapping[(j << 1) + 1] = (j << 1) + 1;
    }
  }

  void do_connections(const UnsignedInt i, const UnsignedInt offset) {
    if (i == 1)
      return;

    auto &pattern = interstages[n_ - i + 1];

    for (UnsignedInt j = 0; j < (1 << (i - 2)); ++j) {
      // upper ports of the upper switches shall be connected to
      // the upper network
      pattern.mapping[offset + (j << 1)] = offset + (j << 1);

      // lower ports of the lower switches shall be connected to
      // the lower network
      pattern.mapping[offset + (1 << (i - 1)) + (j << 1) + 1] =
          offset + (1 << (i - 1)) + (j << 1) + 1;

      // lower ports of the upper switches shall be connected to
      // the lower network
      pattern.mapping[offset + (j << 1) + 1] =
          offset + (j << 1) + ((1 << (i - 1)) - 1) + 1;

      // upper ports of the lower switches shall be connected to
      // the upper network
      pattern.mapping[offset + (1 << (i - 1)) + (j << 1)] =
          offset + (1 << (i - 1)) + (j << 1) - ((1 << (i - 1)) - 1);
    }

    do_connections(i - 1, offset);
    do_connections(i - 1, offset + (1 << (i - 1)));
  }

  void do_boolean_splitting_send_packet(UnsignedInt stage_idx,
                                        UnsignedInt mask1, UnsignedInt mask2,
                                        UnsignedInt mask3, UnsignedInt i,
                                        std::pair<UnsignedInt, UnsignedInt> p) {
    if (!mask1)
      return;

    auto &stage = stages[stage_idx];

    if (((p.first ^ p.second) & mask1) == 0) {
      // case I
      if (p.first & mask1) {
        // mk == Mk == 1, send via lower link
        if (i % 2) {
          stage.switch_states[i >> 1].make_straight();

          do_boolean_splitting_send_packet(
              stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
              interstages[stage_idx + 1].mapping[i], p);
        } else {
          stage.switch_states[i >> 1].make_cross();

          do_boolean_splitting_send_packet(
              stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
              interstages[stage_idx + 1].mapping[i + 1], p);
        }
      } else {
        // mk == Mk == 0, send via upper link
        if (i % 2) {
          stage.switch_states[i >> 1].make_cross();

          do_boolean_splitting_send_packet(
              stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
              interstages[stage_idx + 1].mapping[i - 1], p);
        } else {
          stage.switch_states[i >> 1].make_straight();

          do_boolean_splitting_send_packet(
              stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
              interstages[stage_idx + 1].mapping[i], p);
        }
      }
    } else /* mk == 0 and Mk == 1 */ {
      std::pair<UnsignedInt, UnsignedInt> upper, lower;

      upper.first = p.first;
      upper.second = (p.second & ~mask3) | mask2;

      lower.first = upper.second + 1;
      lower.second = p.second;

      if (i % 2) {
        stage.switch_states[i >> 1].make_lowerbroadcast();

        do_boolean_splitting_send_packet(
            stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
            interstages[stage_idx + 1].mapping[i - 1], upper);

        do_boolean_splitting_send_packet(
            stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
            interstages[stage_idx + 1].mapping[i], lower);
      } else {
        stage.switch_states[i >> 1].make_upperbroadcast();

        do_boolean_splitting_send_packet(
            stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
            interstages[stage_idx + 1].mapping[i], upper);

        do_boolean_splitting_send_packet(
            stage_idx + 1, mask1 >> 1, mask2 >> 1, mask3 >> 1,
            interstages[stage_idx + 1].mapping[i + 1], lower);
      }
    }
  }
};

} // namespace multistage_interconnect

#endif // BANYAN_HPP_INCLUDED
