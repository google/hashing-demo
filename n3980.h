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

// Extensions to namespace std to implement N3980. Not part of this proposal,
// but implemented as a basis for comparison. Currently contains just enough
// to get the benchmark working.

#ifndef HASHING_DEMO_N3980_H
#define HASHING_DEMO_N3980_H

#include <string>
#include <type_traits>

namespace std {

template <typename HashAlgorithm, typename Integral>
enable_if_t<is_integral<Integral>::value || is_enum<Integral>::value>
hash_append(HashAlgorithm& h, Integral value) {
  h(&value, sizeof(value));
}

template <typename HashAlgorithm>
void hash_append(HashAlgorithm& h, const string& str) {
  h(str.data(), str.size());
  hash_append(h, str.size());
}

template <typename H>
struct uhash {
  using result_type = typename H::result_type;

  template <typename T>
  result_type operator()(const T& t) {
    H h;
    using std::hash_append;
    hash_append(h, t);
    return static_cast<result_type>(h);
  }
};

}  // namespace std

#endif  // HASHING_DEMO_N3980_H
