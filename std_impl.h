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

#ifndef HASHING_DEMO_STD_IMPL_H
#define HASHING_DEMO_STD_IMPL_H
// This file contains the proposed members of namespace std that are needed
// to define a hash algorithm. Hash algorithm headers that are intended
// to be usable by std_::hash should include this header rather than
// std.h, to avoid circular dependencies.

#include <cstddef>
#include <forward_list>
#include <memory>
#include <string>
#include <vector>

namespace std_ {

// Make std_ look as much like std as possible.
using std::array;
using std::declval;
using std::enable_if_t;
using std::false_type;
using std::forward_list;
using std::get;
using std::index_sequence;
using std::integral_constant;
using std::is_enum;
using std::is_floating_point;
using std::is_integral;
using std::make_index_sequence;
using std::nullptr_t;
using std::pair;
using std::string;
using std::true_type;
using std::tuple;
using std::unique_ptr;
using std::vector;

// is_uniquely_represented type trait
// ==========================================================================
template <typename T, typename Enable = void>
struct is_uniquely_represented : false_type {};

// The standard must guarantee that this specialization is present, and
// true, so that hash_value() recursion is guaranteed to eventually
// reach a uniquely-represented type.
template <>
struct is_uniquely_represented<unsigned char> : true_type {};

// All specializations of this template from here down are effectively
// optimizations, and so their presence is a QOI issue. The following
// code is not strictly portable, since the current standard doesn't
// guarantee that any of these types are uniquely represented, but I
// believe this will work on most common platforms.

template <typename T>
struct is_uniquely_represented<T, enable_if_t<is_integral<T>::value>>
    : true_type {};

// We might actually be able to omit this for implementations that use the
// Itanium ABI, but better safe than sorry.
template <>
struct is_uniquely_represented<bool> : public false_type {};

template <typename T>
struct is_uniquely_represented<T*> : public true_type {};

template <typename T, size_t N>
struct is_uniquely_represented<T[N]> : public is_uniquely_represented<T> {};

template <typename T>
struct is_uniquely_represented<T, enable_if_t<is_enum<T>::value>>
    : public true_type {};

namespace detail {
template <typename... Ts>
struct all_uniquely_represented;

template <typename T, typename... Ts>
struct all_uniquely_represented<T, Ts...>
    : public integral_constant<bool,
                               is_uniquely_represented<T>::value &&
                                   all_uniquely_represented<Ts...>::value> {};
template <>
struct all_uniquely_represented<> : public true_type {};

template <typename... Ts>
struct cumulative_size;

template <typename T, typename... Ts>
struct cumulative_size<T, Ts...>
    : public integral_constant<size_t,
                               sizeof(T) + cumulative_size<Ts...>::value> {};
template <>
struct cumulative_size<> : public integral_constant<size_t, 0> {};
}  // namespace detail

template <typename T, typename U>
struct is_uniquely_represented<pair<T, U>>
    : public integral_constant<bool, is_uniquely_represented<T>::value &&
                                         is_uniquely_represented<U>::value &&
                                         (sizeof(T) + sizeof(U) ==
                                          sizeof(pair<T, U>))> {};

template <typename... Ts>
struct is_uniquely_represented<tuple<Ts...>>
    : public integral_constant<bool,
                               detail::all_uniquely_represented<Ts...>::value &&
                                   (detail::cumulative_size<Ts...>::value ==
                                    sizeof(tuple<Ts...>))> {};

template <typename T, size_t N>
struct is_uniquely_represented<array<T, N>>
    : public integral_constant<bool, is_uniquely_represented<T>::value &&
                               sizeof(T[N]) == sizeof(array<T, N>)> {};

// hash_value function overloads for standard types
// ==========================================================================

namespace detail {
template <typename HashCode, typename T>
HashCode hash_bytes(HashCode code, const T& value) {
  const unsigned char* start = reinterpret_cast<const unsigned char*>(&value);
  return hash_combine_range(std::move(code), start, start + sizeof(value));
}

// Requires: Container has begin(), end(), and size() methods
// If N4183 is not available (see below), we would need to handle contiguous
// containers separately, for efficiency.
template <typename HashCode, typename Container>
HashCode hash_sized_container(
    HashCode code, const Container& container) {
  // Following N3980, we append the container size to the hash of all
  // containers.
  return hash_combine(
      hash_combine_range(
          std::move(code), container.begin(), container.end()),
      // Force size_t so that the choice of container doesn't affect the
      // hash value.
      static_cast<size_t>(container.size()));
}
}  // namespace detail

template <typename HashCode, typename Integral>
enable_if_t<is_integral<Integral>::value || is_enum<Integral>::value,
            HashCode>
hash_value(HashCode code, Integral value) {
  return detail::hash_bytes(std::move(code), value);
}

template <typename HashCode>
HashCode hash_value(HashCode code, bool value) {
  return hash_combine(std::move(code),
                      static_cast<unsigned char>(value ? 1 : 0));
}

template <typename HashCode, typename Float>
enable_if_t<is_floating_point<Float>::value,
            HashCode>
hash_value(HashCode code, Float value) {
  return detail::hash_bytes(std::move(code), value == 0 ? 0 : value);
}

template <typename HashCode, typename T>
HashCode hash_value(HashCode code, T* ptr) {
  return detail::hash_bytes(std::move(code), ptr);
}

template <typename HashCode>
HashCode hash_value(HashCode code, nullptr_t p) {
  return hash_combine(code, static_cast<unsigned char>(0));
}

template <typename HashCode, typename T>
HashCode hash_value(HashCode code, const vector<T>& v) {
  return detail::hash_sized_container(std::move(code), v);
}

template <typename HashCode>
HashCode hash_value(HashCode code, const string& s) {
  return detail::hash_sized_container(std::move(code), s);
}

// TODO: similar overloads for deque, list, set, map, multiset, multimap.
// Unordered containers are omitted as discussed in N3980. C-style arrays
// are omitted because they seem unlikely to be useful, and it's not entirely
// clear whether the size should be hashed.

// I've chosen to treat std::array as a container rather than a
// tuple-like type, meaning that the hash includes the size.
template <typename HashCode, typename T, size_t N>
HashCode hash_value(HashCode code, const array<T, N>& a) {
  return detail::hash_sized_container(std::move(code), a);
}

template <typename HashCode, typename T>
HashCode hash_value(HashCode code, const forward_list<T>& l) {
  // We traverse the list manually rather than calling hash_combine_range,
  // so that we can compute the size without a second traversal. As with
  // hash_sized_container, we use size_t rather than the container's
  // size_type, for cross-container consistency.
  size_t size = 0;
  for (const T& t : l) {
    code = hash_combine(std::move(code), t);
    ++size;
  }
  return hash_combine(std::move(code), size);
}

template <typename HashCode, typename T, typename D>
HashCode hash_value(HashCode code, const unique_ptr<T,D>& ptr) {
  return hash_combine(std::move(code), ptr.get());
}

template <typename HashCode, typename T, typename U>
HashCode hash_value(HashCode code, const pair<T,U>& p) {
  return hash_combine(std::move(code), p.first, p.second);
}

namespace detail {
template <typename HashCode, typename Tuple, size_t... Is>
HashCode hash_tuple(HashCode code, const Tuple& t, index_sequence<Is...>) {
  return hash_combine(std::move(code), get<Is>(t)...);
}
}  // namespace detail

template <typename HashCode, typename... Ts>
HashCode hash_value(HashCode code, const tuple<Ts...>& t) {
  return detail::hash_tuple(
      std::move(code), t, make_index_sequence<sizeof...(Ts)>());
}

// Dummy implementation of N4183 (contiguous iterator utilities), so
// that we can show examples of code that uses it. N4183 is independent
// of this proposal, but they synergize well.
// ==========================================================================

template <typename T>
struct is_contiguous_iterator : public false_type {};

template <typename T>
struct is_contiguous_iterator<T*> : public true_type {};

template <typename T>
T* adl_pointer_from(T* ptr) { return ptr; }

template <>
struct is_contiguous_iterator<string::iterator> : public true_type {};

inline char* adl_pointer_from(string::iterator i) { return &*i; }

template <>
struct is_contiguous_iterator<string::const_iterator>
    : public true_type {};

inline const char* adl_pointer_from(string::const_iterator i) { return &*i; }

// Implementation of N3911, provided here for convenience.
// ==========================================================================

template< class... > using void_t = void;

}  // namespace std_

#endif   // HASHING_DEMO_STD_IMPL_H
