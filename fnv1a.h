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

  friend fnv1a hash_combine(
      fnv1a hash_code, std::iterator_range<const unsigned char*> range) {
    for (unsigned char c : range) {
      hash_code.state_ = (hash_code.state_ ^ c) * 1099511628211u;
    }
    return hash_code;
  }

  explicit operator result_type() && noexcept { return state_; }
};

}  // namespace hashing

#endif  // HASHING_DEMO_FNV1A_H
