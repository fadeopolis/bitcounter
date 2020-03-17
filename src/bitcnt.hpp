
#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint8_t
#include <memory>  // std::unique_ptr

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

/// Data must be aligned to 64 bytes
Count bitcount(size_t size, const uint8_t *data);

/// Wrapper for data properly aligned for bitcount().
struct Bitcount_Buffer {
  static Bitcount_Buffer allocate(size_t size);

  Bitcount_Buffer(Bitcount_Buffer &&that) : _ptr{that._ptr} { that._ptr = nullptr; }
  Bitcount_Buffer(const Bitcount_Buffer&) = delete;
  ~Bitcount_Buffer();

  uint8_t *get() { return _ptr; }
private:
  explicit Bitcount_Buffer(uint8_t *ptr) : _ptr{ptr} {}

  uint8_t *_ptr;
};

} // end namespace bc
