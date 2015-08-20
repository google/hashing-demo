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

#include "std_impl.h"

namespace hashing {

// Implementation of FNV-1a hash algorithm, based on the implementation
// in N3980.
class fnv1a {
  std::size_t state_ = 14695981039346656037u;

 public:
  struct state_type{};
  using result_type = size_t;

  fnv1a(state_type* /* unused */) {}

  friend fnv1a hash_combine_range(
      fnv1a hash_code, const unsigned char* begin, const unsigned char* end) {
    while (begin < end) {
      hash_code.state_ = mix(hash_code, *begin);
      ++begin;
    }
    return hash_code;
  }

  explicit operator result_type() && noexcept { return state_; }

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
  static constexpr bool hashes_exact_representation = true;
  struct state_type {};
  using result_type = size_t;

  type_invariant_fnv1a(state_type* /* unused */) {}

  type_invariant_fnv1a(const type_invariant_fnv1a&) = delete;
  type_invariant_fnv1a& operator=(const type_invariant_fnv1a&) = delete;
  type_invariant_fnv1a(type_invariant_fnv1a&&) = default;
  type_invariant_fnv1a& operator=(type_invariant_fnv1a&&) = default;

  friend type_invariant_fnv1a hash_combine_range(
      type_invariant_fnv1a hash_code, unsigned char const* begin,
      unsigned char const* end) {
    while (begin < end) {
      hash_code.state_ = mix(std::move(hash_code), *begin);
      ++begin;
    }
    return hash_code;
  }

  explicit operator result_type() noexcept { return state_; }

 private:
  type_invariant_fnv1a(result_type state) : state_(state) {}

  static size_t mix(type_invariant_fnv1a hash_code, unsigned char c) {
    return (hash_code.state_ ^ c) * 1099511628211u;
  }

};

}  // namespace hashing

#endif  // HASHING_DEMO_FNV1A_H
