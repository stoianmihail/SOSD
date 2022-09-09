#pragma once

#include "../util.h"
#include "base.h"
#include "GENE/src/optimized/Index.hpp"
#include <cassert>

template <class KeyType, int size_scale>
class Gene : public Competitor {
 public:
  uint64_t Build(const std::vector<KeyValue<KeyType>>& data) {
    std::vector<std::pair<KeyType, uint64_t>> loading_data;
    loading_data.reserve(data.size());
    // We use GENE as a non-clustered index by only inserting every n-th entry.
    // n is defined by size_scale.
    for (auto& itm : data) {
      uint64_t idx = itm.value;
      if (size_scale > 1 && idx % size_scale != 0) continue;
      loading_data.push_back(std::make_pair(itm.key, itm.value));
    }

    data_size_ = data.size();

    return util::timing([&] {
      map_ = opt::Index<key_type, mapped_type>
        ::Bulkload(data.begin(), data.end(),
        inner_slot_size + 1, leaf_slot_size);
    });
  }

  SearchBound EqualityLookup(const KeyType lookup_key) const {
    auto raw_guess = map_.lower_bound(lookup_key);
    assert(raw_guess.has_value());
    auto guess = raw_guess.value();
    assert(guess < data_size_);

    const uint64_t error = size_scale - 1;
    const uint64_t start = guess < error ? 0 : guess - error;
    const uint64_t stop = guess + 1 > data_size_
                              ? data_size_
                              : guess + 1;  // stop is exclusive (that's why +1)

    return (SearchBound){start, stop};
  }

  std::string name() const { return "GENE"; }

  std::size_t size() const { return map_.index_size(); }

  int variant() const { return size_scale; }

 private:
  uint64_t data_size_ = 0;
  opt::Index<KeyType, uint64_t> map_;
};
