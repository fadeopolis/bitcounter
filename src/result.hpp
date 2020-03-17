
#pragma once

#include <cassert>     // assert
#include <variant>     // std::variant
#include <type_traits> // std::is_same_v

namespace bc {

template<typename T, typename Err>
struct Result final {
  using value_type = T;
  using error_type = Err;

  static_assert(!std::is_same_v<value_type, error_type>);

  /// ctor/dtor

  Result(const T &val) : _data{std::in_place_type<value_type>, val} {}

  Result(const Err &err) : _data{std::in_place_type<error_type>, err} {}

  /// observers

  bool has_value() const {
    assert(!_data.valueless_by_exception());
    return std::holds_alternative<value_type>(_data);
  }

  bool has_error() const {
    assert(!_data.valueless_by_exception());
    return std::holds_alternative<error_type>(_data);
  }

  T &get_value() {
    assert(has_value());
    return std::get<value_type>(_data);
  }

  const T &get_value() const {
    assert(has_value());
    return std::get<value_type>(_data);
  }

  T &get_value_or(T &default_) {
    return (has_value()) ? get_value() : default_;
  }

  const T &get_value_or(const T &default_) const {
    return (has_value()) ? get_value() : default_;
  }

  Err &get_error() {
    assert(has_error());
    return std::get<error_type>(_data);
  }

  const Err &get_error() const {
    assert(has_error());
    return std::get<error_type>(_data);
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
  typename std::variant<T, Err> _data;
};

} // end namespace bc
