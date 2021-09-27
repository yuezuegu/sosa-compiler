#ifndef ENTRY_HPP_INCLUDED
#define ENTRY_HPP_INCLUDED

#include "banyan.hpp"
#include "benes.hpp"
#include <cassert>
#include <map>

using multistage_interconnect::Int;
using multistage_interconnect::UnsignedInt;

template <typename Interconnect>
inline void propagate_impl(Interconnect &interconnect, Int const *packets,
                           Int *output, Int invalid) {
  auto N = 1 << interconnect.n();
  std::vector<Int> packets_vector(packets, packets + N);
  auto &&propagated = interconnect.propagate(packets_vector, invalid);
  for (std::size_t i = 0; i < N; ++i) {
    output[i] = propagated[i];
  }
}

struct InterconnectBase {
  virtual UnsignedInt num_ports() const = 0;
  virtual UnsignedInt latency() const = 0;
  virtual void reset() = 0;
  virtual void propagate(Int const *packets, Int *output,
                         Int invalid) const = 0;
  virtual bool do_apply_permute(Int const *inverse_mapping) = 0;
  virtual bool do_is_route_free(UnsignedInt src, UnsignedInt dest) = 0;
  virtual InterconnectBase *clone() const = 0;
  virtual ~InterconnectBase() = default;

  virtual UnsignedInt data_req_latency() const {
    return latency();
  }

  virtual UnsignedInt data_read_latency() const {
    return latency() + 1;
  }

  virtual UnsignedInt total_latency() const {
    return data_req_latency() + data_read_latency();
  }

  template <typename BankType, typename TargetType>
  bool apply_permute(std::map<BankType *, TargetType *> *permute) {
    std::vector<Int> v(num_ports(), -1);
    for (auto it = cbegin(*permute); it != cend(*permute); ++it) {
      v[it->first->id] = it->second->id;
    }
    return do_apply_permute(&v[0]);
  }

  template <typename BankType, typename TargetType>
  bool is_route_free(BankType *bank, TargetType * target) {
    return do_is_route_free(bank->id, target->id);
  }
};

struct Benes : InterconnectBase {
  enum Algo : Int {
    ALGO_DUMMY = 0,
    ALGO_RAGHAVENDRA = 1,
    ALGO_LOOPING_MULTICAST = 2
  };

  multistage_interconnect::Benes impl;
  std::vector<Int> current_inverse_mapping;
  Algo algorithm = ALGO_LOOPING_MULTICAST;
  UnsignedInt trials = 10;

  Benes(UnsignedInt n) : impl{n}, current_inverse_mapping(1 << n, -1) {}

  UnsignedInt num_ports() const override {
    return current_inverse_mapping.size();
  }

  UnsignedInt latency() const override { return impl.n() * 2 - 1; }

  void reset() override {
    impl.reset();
    for (auto &x : current_inverse_mapping)
      x = -1;
  }

  void propagate(Int const *packets, Int *output, Int invalid) const override {
    propagate_impl(impl, packets, output, invalid);
  }

  void set_algorithm(Algo algorithm) { this->algorithm = algorithm; }

  void set_trials(UnsignedInt trials) { this->trials = trials; }

  bool do_apply_permute(Int const *inverse_mapping) override {
    for (UnsignedInt i = 0; i < current_inverse_mapping.size(); ++i) {
      current_inverse_mapping[i] = inverse_mapping[i];
    }

    impl.reset();

    switch (algorithm) {
    case ALGO_DUMMY: {
      return impl.dummy(&current_inverse_mapping[0]);
    }
    case ALGO_RAGHAVENDRA: {
      return impl.raghavendra(&current_inverse_mapping[0]);
    }
    case ALGO_LOOPING_MULTICAST: {
      return impl.looping_multicast(&current_inverse_mapping[0], trials);
    }
    }

    throw std::runtime_error("not valid default algorithm type");
  }

  bool do_is_route_free(UnsignedInt src, UnsignedInt dest) override {
    if (current_inverse_mapping[dest] != -1)
      return false;

    auto temp = current_inverse_mapping;
    temp[dest] = src;

    impl.reset();

    switch (algorithm) {
    case ALGO_DUMMY: {
      return impl.dummy(&temp[0]);
    }
    case ALGO_RAGHAVENDRA: {
      return impl.raghavendra(&temp[0]);
    }
    case ALGO_LOOPING_MULTICAST: {
      return impl.looping_multicast(&temp[0], trials);
    }
    }

    throw std::runtime_error("not valid default algorithm type");
  }

  Benes *clone() const override { return new Benes(*this); }
};

struct BenesWithCopy : InterconnectBase {
  BenesWithCopy(UnsignedInt n) : n_{n} {}

  UnsignedInt latency() const override { return n_ * 4 - 1; }

  UnsignedInt num_ports() const override { return 1 << n_; }

  void reset() override {}

  void propagate(Int const *packets, Int *output, Int invalid) const override {}

  bool do_apply_permute(Int const *inverse_mapping) override { return true; }

  bool do_is_route_free(UnsignedInt src, UnsignedInt dest) override {
    return true;
  }

  BenesWithCopy *clone() const override { return new BenesWithCopy(*this); }

private:
  UnsignedInt n_;
};

struct Banyan : InterconnectBase {
  multistage_interconnect::Banyan impl;
  std::vector<Int> current_inverse_mapping;

  // expansion is given logarithmically
  UnsignedInt expansion = 0;

  Banyan(UnsignedInt n) : impl{n}, current_inverse_mapping(1 << n, -1) {}

  UnsignedInt latency() const override { return impl.n() + expansion; }

  UnsignedInt num_ports() const override {
    return current_inverse_mapping.size();
  }

  void reset() override {
    impl.reset();
    for (auto &x : current_inverse_mapping)
      x = -1;
  }

  void propagate(Int const *packets, Int *output, Int invalid) const override {
    propagate_impl(impl, packets, output, invalid);
  }

  void set_expansion(UnsignedInt expansion) { this->expansion = expansion; }

  // TODO most probably, we can do it in a faster way without
  // current_inverse_mapping
  bool do_apply_permute(Int const *inverse_mapping) override {
    for (UnsignedInt i = 0; i < current_inverse_mapping.size(); ++i) {
      current_inverse_mapping[i] = inverse_mapping[i];
    }

    impl.reset();

    if (expansion == 0)
      return impl.bit_follow(&current_inverse_mapping[0], true);
    else
      return impl.bit_follow_expansion(&current_inverse_mapping[0], expansion);
  }

  bool do_is_route_free(UnsignedInt src, UnsignedInt dest) override {
    if (current_inverse_mapping[dest] != -1)
      return false;

    auto temp = current_inverse_mapping;
    temp[dest] = src;

    impl.reset();

    if (expansion == 0)
      return impl.bit_follow(&temp[0], true);
    else
      return impl.bit_follow_expansion(&temp[0], expansion);
  }

  Banyan *clone() const override { return new Banyan(*this); }
};

#endif // ENTRY_HPP_INCLUDED
