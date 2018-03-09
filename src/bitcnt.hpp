
#pragma once

#include "slice.hpp"
#include <stddef.h>

namespace bc {

struct Count final {
  size_t ones = 0;
  size_t zeroes = 0;

  Count &operator+=(Count c) {
    this->ones   += c.ones;
    this->zeroes += c.zeroes;
    return *this;
  }
  Count operator+(Count c) const {
    Count tmp{*this};
    tmp += c;
    return tmp;
  }

  size_t bits() const {
    return ones + zeroes;
  }

  double percent_ones() const {
    return double(ones) / bits();
  }

  double percent_zeroes() const {
    return double(zeroes) / bits();
  }
};

Count bitcount(Bytes data);

} // end namespace bc
