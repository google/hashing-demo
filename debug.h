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
  std::string hash_input_;

 public:
  using result_type = std::string;

  identity() {}

  identity(const identity&) = delete;
  identity& operator=(const identity&) = delete;
  identity(identity&&) = default;
  identity& operator=(identity&&) = default;

  template <typename T, typename... Ts>
  friend identity hash_combine(
      identity code, const T& value, const Ts&... values) {
    using std_::hash_value;
    return hash_combine(hash_value(std::move(code), value), values...);
  }

  template <typename... Ts>
  friend identity hash_combine(
      identity code, unsigned char c, const Ts&... values) {
    code.hash_input_.push_back(c);
    return hash_combine(std::move(code), values...);
  }

  friend identity hash_combine(identity code) {
    return std::move(code);
  }

  template <typename InputIterator>
  friend identity hash_combine_range(
      identity code, InputIterator begin, InputIterator end) {
    using std_::hash_value;
    while (begin != end) {
      code = hash_value(std::move(code), *begin);
      ++begin;
    }
    return code;
  }

  friend identity hash_combine_range(identity code, const unsigned char* begin,
      const unsigned char* end) {
    code.hash_input_.append(begin, end);
    return std::move(code);
  }

  explicit operator result_type() && {
    return result_type(std::move(hash_input_));
  }
};




}  // namespace hashing
