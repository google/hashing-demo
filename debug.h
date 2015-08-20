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
  struct state_type {};
  using result_type = std::basic_string<unsigned char>;

  identity(state_type* /* unused */) {}

  identity(const identity&) = delete;
  identity& operator=(const identity&) = delete;
  identity(identity&&) = default;
  identity& operator=(identity&&) = default;

  template <typename T, typename U, typename... Ts>
  friend identity hash_combine(
      identity code, const T& t, const U& u, const Ts&... values) {
    return hash_combine_impl(std::move(code), t, u, values...);
  }
  template <typename InputIterator>
  friend identity hash_combine_range(
      identity code, InputIterator begin, InputIterator end) {
    using std::hash_combine;
    while (begin != end) {
      code = hash_combine(std::move(code), *begin);
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

 private:
  template <typename T, typename... Ts>
  static identity hash_combine_impl(
      identity code, const T& value, const Ts&... values) {
    using std::hash_combine;
    return hash_combine_impl(hash_combine(std::move(code), value), values...);
  }

  template <typename... Ts>
  static identity hash_combine_impl(
      identity code, unsigned char c, const Ts&... values) {
    code.hash_input_.push_back(c);
    return hash_combine_impl(std::move(code), values...);
  }

  static identity hash_combine_impl(identity code) {
    return std::move(code);
  }


};




}  // namespace hashing
