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

#include "std.h"

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

template <typename HashAlgorithm, typename T1, typename T2>
void hash_append(HashAlgorithm& h, const pair<T1, T2>& p) {
  hash_append(h, p.first);
  hash_append(h, p.second);
}

template <typename HashAlgorithm, typename T, typename... Ts>
void hash_append(HashAlgorithm& h, const T& value, const Ts&... values) {
  hash_append(h, value);
  hash_append(h, values...);
}

namespace detail {
template <typename HashAlgorithm, typename Tuple, size_t... Is>
void hash_append_tuple(
    HashAlgorithm& h, const Tuple& t, index_sequence<Is...>) {
  hash_append(h, get<Is>(t)...);
}
}  // namespace detail

template <typename HashAlgorithm, typename... Ts>
void hash_append(HashAlgorithm& h, const tuple<Ts...>& t) {
  detail::hash_append_tuple(h, t, make_index_sequence<sizeof...(Ts)>());
}

namespace detail {
struct hash_bytes_directly_tag {};
struct hash_by_iterating_tag {};

template <typename C>
struct select_hash_iteration_strategy {
  using type = hash_by_iterating_tag;
};

template <typename T>
struct select_hash_iteration_strategy<vector<T>> {
  using type = conditional_t<is_uniquely_represented<T>::value,
                             hash_bytes_directly_tag, hash_by_iterating_tag>;
};

template <typename HashAlgorithm, typename C>
void hash_append_container(
    HashAlgorithm& h, const C& c, hash_bytes_directly_tag) {
  if (!c.empty()) {
    h(&*c.begin(), c.size() * sizeof(*c.begin()));
  }
  hash_append(h, static_cast<size_t>(c.size()));
}

template <typename HashAlgorithm, typename C>
void hash_append_container(
    HashAlgorithm& h, const C& c, hash_by_iterating_tag) {
  for (const auto& v : c) {
    hash_append(h, v);
  }
}
}  // namespace detail

template <typename HashAlgorithm, typename T>
void hash_append(HashAlgorithm& h, const vector<T>& v) {
  detail::hash_append_container(
      h, v, typename detail::select_hash_iteration_strategy<vector<T>>::type());
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
