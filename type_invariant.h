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

#ifndef HASHING_DEMO_TYPE_INVARIANT_H
#define HASHING_DEMO_TYPE_INVARIANT_H

#include <utility>

namespace hashing {

// Wrapper for type-invariant hashing
template <typename HashCode>
class type_invariant_hash {
 private:
  HashCode impl_;

 public:
  static constexpr bool hashes_exact_representation = true;
  using state_type = typename HashCode::state_type;
  using result_type = typename HashCode::result_type;

  type_invariant_hash(state_type* state) : impl_(state) {}

  type_invariant_hash(const type_invariant_hash&) = delete;
  type_invariant_hash& operator=(const type_invariant_hash&) = delete;
  type_invariant_hash(type_invariant_hash&&) = default;
  type_invariant_hash& operator=(type_invariant_hash&&) = default;

  friend type_invariant_hash hash_combine_range(
      type_invariant_hash hash_code, unsigned char const* begin,
      unsigned char const* end) {
    using std::hash_combine_range;
    return type_invariant_hash(hash_combine_range(
        std::move(hash_code.impl_), begin, end));
  }

  explicit operator result_type() && noexcept {
    return static_cast<result_type>(std::move(impl_));
  }

 private:
  type_invariant_hash(HashCode impl) : impl_(std::move(impl)) {}
};

}  // namespace

#endif
