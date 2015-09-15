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

#ifndef HASHING_DEMO_TYPE_ERASED_HASH_CODE_H
#define HASHING_DEMO_TYPE_ERASED_HASH_CODE_H

#include <functional>
#include <memory>
#include <utility>

#include "std.h"

namespace hashing {

class type_erased_hash_code {
  std::function<void(unsigned char const*, unsigned char const*)> wrapper_;

  template <typename HashCode>
  struct HashCodeWrapper {
    HashCode* hash_code_;

    void operator()(unsigned char const* begin, unsigned char const* end) {
      *hash_code_ = hash_combine_range(std::move(*hash_code_), begin, end);
    }
  };

 public:
  template <typename HashCode>
  type_erased_hash_code(HashCode* hash_code)
      : wrapper_(HashCodeWrapper<HashCode>{hash_code}) {}

  friend type_erased_hash_code hash_combine_range(
      type_erased_hash_code hash_code, unsigned char const* begin,
      unsigned char const* end) {
    hash_code.wrapper_(begin, end);
    return std::move(hash_code);
  }
};

// Various optimizations of these overloads are possible, but omitted for
// simplicity.

template <typename T, typename... Ts>
type_erased_hash_code hash_combine(
    type_erased_hash_code hash_code, const T& value, const Ts&... values) {
  using std_::hash_value;
  return hash_combine(hash_value(std::move(hash_code), value), values...);
}

inline type_erased_hash_code hash_combine(type_erased_hash_code hash_code) {
  return hash_code;
}

template <typename InputIterator>
type_erased_hash_code hash_combine_range(
    type_erased_hash_code hash_code, InputIterator begin, InputIterator end) {
  while (begin != end) {
    using std_::hash_value;
    hash_code = hash_value(std::move(hash_code), *begin);
    ++begin;
  }
  return hash_code;
}

}  // namespace hashing

#endif // HASHING_DEMO_TYPE_ERASED_HASH_CODE_H
