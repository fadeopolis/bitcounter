
#pragma once

#include <cassert>
#include <variant>

namespace bc {

template<typename T, typename Err>
struct Result final {
  /// ctor/dtor

  Result(const T &t) : _data{std::in_place_index<value_index()>, t} {}

  Result(const Err &err) : _data{std::in_place_index<error_index()>, err} {}

  /// observers

  bool has_value() const {
    assert(!_data.valueless_by_exception());
    return _data.index() == value_index();
  }

  bool has_error() const {
    assert(!_data.valueless_by_exception());
    return _data.index() == error_index();
  }

  T &get_value() {
    assert(has_value());
    return std::get<value_index()>(_data);
  }

  const T &get_value() const {
    assert(has_value());
    return std::get<value_index()>(_data);
  }

  T &get_value_or(T &default_) {
    return (has_value()) ? get_value() : default_;
  }

  const T &get_value_or(const T &default_) const {
    return (has_value()) ? get_value() : default_;
  }

  Err &get_error() {
    assert(has_error());
    return std::get<error_index()>(_data);
  }

  const Err &get_error() const {
    assert(has_error());
    return std::get<error_index()>(_data);
  }

  explicit operator bool() const {
    return has_value();
  }

  T *operator->() {
    return &get_value();
  }

  const T *operator->() const {
    return &get_value();
  }

  const T &operator*() {
    return get_value();
  }

  const T &operator*() const {
    return get_value();
  }
private:
  constexpr static inline size_t value_index() { return 0; }
  constexpr static inline size_t error_index() { return 1; }

  typename std::variant<T, Err> _data;
};

} // end namespace bc
