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

#include <string>

#include "std.h"

namespace hashing {

class identity {
  std::basic_string<unsigned char> hash_input_;

 public:
  static constexpr bool hashes_exact_representation = true;
  struct state_type {};
  using result_type = std::basic_string<unsigned char>;

  identity(state_type* /* unused */) {}

  identity(const identity&) = delete;
  identity& operator=(const identity&) = delete;
  identity(identity&&) = default;
  identity& operator=(identity&&) = default;

  friend identity hash_combine(
      identity code, std::iterator_range<const unsigned char*> range) {
    code.hash_input_.append(range.begin(), range.end());
    return std::move(code);
  }

  explicit operator result_type() && {
    return result_type(std::move(hash_input_));
  }
};




}  // namespace hashing
