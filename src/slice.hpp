
#pragma once

#include "types.hpp"
#include <algorithm>
#include <cstddef>
#include <cassert>

namespace bc {

/// Immutable array of __restrict__ data.
template<typename T>
struct Slice final {
  using value_type     = T;
  using const_iterator = const T*;
  using iterator       = const_iterator;

  /// ctor

  constexpr explicit Slice(size_t size, const T *data) : _size{size}, _data{data} {}

  constexpr explicit Slice() : Slice{0, nullptr} {}

  constexpr explicit Slice(const T &t) : Slice{1, &t} {}

  template<int size>
  constexpr Slice(const T(&arr)[size]) : Slice{size, arr} {}

  /// observers

  constexpr size_t size() const { return _size; }

  constexpr size_t empty() const { return _size == 0; }

  constexpr size_t not_empty() const { return _size != 0; }

  constexpr const T &operator[](size_t idx) const {
    assert(idx < _size);
    return _data[idx];
  }

  constexpr const_iterator data() const { return _data; }

  /// iteration

  constexpr const_iterator begin() const { return _data; }
  constexpr const_iterator end()   const { return _data + _size; }

  /// comparison

  template<int size>
  bool operator==(const T(&arr)[size]) const {
    return (size == _size) && std::equal(begin(), end(), arr);
  }

  /// conversion

  template<typename U>
  constexpr Slice<U> cast() const {
    assert((_size % sizeof(U)) == 0);
    return Slice<U>{_size / sizeof(U), reinterpret_cast<const U*>(_data)};
  }

  constexpr Slice<T> slice(size_t start, size_t end) const {
    assert(start <= end);
    assert(start <= _size);
    assert(end <= _size);

    return Slice<T>{end - start, _data + start};
  }

  constexpr Slice<T> slice(size_t start) const {
    assert(start <= _size);

    return Slice<T>{_size - start, _data + start};
  }
private:
  size_t   _size;
  const T *__restrict__ _data;
};

using String = Slice<char>;
using Bytes  = Slice<Byte>;

} // end namespace bc
