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

#ifndef HASHING_DEMO_STD_H
#define HASHING_DEMO_STD_H

#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "farmhash.h"

// The bulk of the proposed library is here. It has been separated out so
// that the implementation of std_::hash can use it without creating
// circular dependencies.
#include "std_impl.h"

namespace std_ {

using hash_code = hashing::farmhash;

namespace detail {
// Trait class that detects whether hash_value(std::hash_code, T) is
// well-formed, for use in the SFINAE logic below.
template <typename T, typename = void>
struct supports_hash_value : public false_type {};

template <typename T>
struct supports_hash_value<
    T, void_t<decltype(hash_value(declval<hash_code>(), declval<T>()))>>
    : public true_type {};

}  // namespace detail

template <typename T>
struct hash {
  // Make operator() SFINAE-friendly
  template <typename U = T>
  enable_if_t<detail::supports_hash_value<U>::value,
              size_t>
  operator()(const U& u) {
    hashing::farmhash::state_type state;
    return hashing::farmhash::result_type(
        hash_value(hashing::farmhash{&state}, u));
  }
};

// std_::unordered set uses std_::hash by default. The other unordered
// containers could be aliased similarly.
template <typename Key,
          typename Hash = hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

}  // namespace std_

#endif  // HASHING_DEMO_STD_H
