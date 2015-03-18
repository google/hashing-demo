// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HASHING_DEMO_FNV1A_H
#define HASHING_DEMO_FNV1A_H

#include "std.h"

namespace hashing {

// Implementation of FNV-1a hash algorithm, based on the implementation
// in N3980.
class fnv1a {
  std::size_t state_ = 14695981039346656037u;

 public:
  using result_type = size_t;

  // Generic recursive case of hash_combine.
  template <typename T, typename... Ts>
  friend std::enable_if_t<!std::is_uniquely_represented_v<T>,
                          fnv1a>
  hash_combine(fnv1a hash_code, const T& value, const Ts&... values) {
    return hash_combine(hash_value(hash_code, value), values...);
  }

  // Base case of hash_combine: hash the bytes directly once we reach a
  // uniquely-represented type.
  template <typename T, typename... Ts>
  friend std::enable_if_t<std::is_uniquely_represented_v<T>,
                          fnv1a>
  hash_combine(fnv1a hash_code, const T& value, const Ts&... values) {
    unsigned char const* bytes = static_cast<unsigned char const*>(&value);
    return hash_combine(
        hash_combine_range(hash_code, bytes, bytes + sizeof(value)), values...);
  }

  // Base case of variadic template recursion.
  friend fnv1a hash_combine(fnv1a hash_code) { return hash_code; }

  // Generic iterative implementation of hash_combine_range.
  template <typename InputIterator>
  // Avoid ambiguity with the following overload
  friend std::enable_if_t<
      !(std::is_contiguous_iterator<InputIterator>::value &&
        std::is_uniquely_represented<
            typename std::iterator_traits<InputIterator>::value_type>::value),
      fnv1a>
  hash_combine_range(fnv1a hash_code, InputIterator begin, InputIterator end) {
    while (begin != end) {
      hash_code = hash_value(hash_code, *begin);
      ++begin;
    }
    return hash_code;
  }

  // Overload for a contiguous sequence of a uniquely-represented type: hash
  // the bytes directly. This is an optimization; the overload above would
  // work in these cases as well, but will probably be much less efficient.
  template <typename InputIterator>
  friend std::enable_if_t<
      std::is_contiguous_iterator<InputIterator>::value &&
          std::is_uniquely_represented<
              typename std::iterator_traits<InputIterator>::value_type>::value,
      fnv1a>
  hash_combine_range(fnv1a hash_code, InputIterator begin, InputIterator end) {
    const unsigned char* begin_ptr =
        static_cast<const unsigned char*>(adl_pointer_from(begin));
    const unsigned char* end_ptr =
        static_cast<const unsigned char*>(adl_pointer_from(end));
    while (begin < end) {
      hash_code.state_ = mix(hash_code, *begin);
      ++begin;
    }
    return hash_code;
  }

  explicit operator result_type() noexcept { return state_; }

 private:
  static size_t mix(fnv1a hash_code, unsigned char c) {
    return (hash_code.state_ ^ c) * 1099511628211u;
  }
};

// Type-invariant implementation of FNV-1a hash algorithm. In order to
// provide the type-invariance property, we have to forego the
// optimization for uniquely-represented types, because different
// types may use different internal representations of the same value.
class type_invariant_fnv1a {
  std::size_t state_ = 14695981039346656037u;

 public:
  using result_type = size_t;

  template <typename T, typename... Ts>
  friend type_invariant_fnv1a hash_combine(
      type_invariant_fnv1a hash_code, const T& value, const Ts&... values) {
    return hash_combine(hash_value(hash_code, value), values...);
  }

  template <typename... Ts>
  friend type_invariant_fnv1a hash_combine(
      type_invariant_fnv1a hash_code, unsigned char c, const Ts&... values) {
    return hash_combine(mix(hash_code, c), values...);
  }

  friend type_invariant_fnv1a hash_combine(type_invariant_fnv1a hash_code) {
    return hash_code;
  }

  template <typename InputIterator>
  friend type_invariant_fnv1a hash_combine_range(
      type_invariant_fnv1a hash_code, InputIterator begin, InputIterator end) {
    while (begin != end) {
      hash_code = hash_value(hash_code, *begin);
      ++begin;
    }
    return hash_code;
  }

  friend type_invariant_fnv1a hash_combine_range(
      type_invariant_fnv1a hash_code, unsigned char const* begin,
      unsigned char const* end) {
    while (begin < end) {
      hash_code.state_ = mix(hash_code, *begin);
      ++begin;
    }
    return hash_code;
  }

  explicit operator result_type() noexcept { return state_; }

 private:
  static size_t mix(type_invariant_fnv1a hash_code, unsigned char c) {
    return (hash_code.state_ ^ c) * 1099511628211u;
  }
};

}  // namespace hashing

#endif  // HASHING_DEMO_FNV1A_H
