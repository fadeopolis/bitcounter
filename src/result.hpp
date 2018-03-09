
#pragma once

#include <cassert>
#include <type_traits>

namespace bc {

template<typename T, typename Err>
struct Result final {
  /// ctor/dtor

  Result(const T &t) : _has_value{true} {
    new (&_data) T{t};
  }

  Result(const Err &err) : _has_value{false} {
    new (&_data) Err{err};
  }

  /// observers

  explicit operator bool() const {
    return has_value();
  }

  T *operator->() {
    assert(has_value());
    return reinterpret_cast<T*>(&_data);
  }

  const T *operator->() const {
    assert(has_value());
    return reinterpret_cast<const T*>(&_data);
  }

  const T &operator*() {
    return get_value();
  }

  const T &operator*() const {
    return get_value();
  }

  bool has_value() const { return _has_value; }
  bool has_error() const { return !_has_value; }

  Err &get_error() {
    assert(has_error());
    return *reinterpret_cast<Err*>(&_data);
  }

  const Err &get_error() const {
    assert(has_error());
    return *reinterpret_cast<const Err*>(&_data);
  }

  T &get_value() {
    return *operator->();
  }

  const T &get_value() const {
    return *operator->();
  }

  T &get_value_or(T &default_) {
    return (has_value()) ? get_value() : default_;
  }

  const T &get_value(const T &default_) const {
    return (has_value()) ? get_value() : default_;
  }
private:
  typename std::aligned_union<1, T, Err>::type _data;
  bool _has_value;
};

} // end namespace bc
