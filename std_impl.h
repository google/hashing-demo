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
// to be usable as default_hash should include this header rather than
// std.h, to avoid circular dependencies.

#include <cstddef>
#include <forward_list>
#include <memory>
#include <string>
#include <vector>

namespace std {

// is_uniquely_represented type trait
// ==========================================================================
template <typename T, typename Enable = void>
struct is_uniquely_represented : false_type {};

// The standard must guarantee that this specialization is present, and
// true, so that hash_decompose() recursion is guaranteed to eventually
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
    : public integral_constant<bool, sizeof(T[N]) == sizeof(array<T, N>)> {};

// hash_decompose function overloads for standard types
// ==========================================================================

namespace detail {
template <typename HashCode, typename T>
void hash_bytes(HashCode& code, const T& value) {
  const unsigned char* start = reinterpret_cast<const unsigned char*>(&value);
  hash_combine_range(code, start, start + sizeof(value));
}

// Requires: Container has begin(), end(), and size() methods
// If N4183 is not available (see below), we would need to handle contiguous
// containers separately, for efficiency.
template <typename HashCode, typename Container>
void hash_sized_container(HashCode& code, const Container& container) {
  // Following N3980, we append the container size to the hash of all
  // containers.
  hash_combine_range(code, container.begin(), container.end());
  // Force size_t so that the choice of container doesn't affect the
  // hash value.
  hash_combine(code, static_cast<size_t>(container.size()));
}
}  // namespace detail

template <typename HashCode, typename Integral>
enable_if_t<is_integral<Integral>::value || is_enum<Integral>::value>
hash_decompose(HashCode& code, Integral value) {
  detail::hash_bytes(code, value);
}

template <typename HashCode>
void hash_decompose(HashCode& code, bool value) {
  hash_combine(code, static_cast<unsigned char>(value ? 1 : 0));
}

template <typename HashCode, typename Float>
enable_if_t<is_floating_point<Float>::value>
hash_decompose(HashCode& code, Float value) {
  detail::hash_bytes(code, value == 0 ? 0 : value);
}

template <typename HashCode, typename T>
void hash_decompose(HashCode& code, T* ptr) {
  detail::hash_bytes(code, ptr);
}

template <typename HashCode>
void hash_decompose(HashCode& code, nullptr_t p) {
  hash_combine(code, static_cast<unsigned char>(0));
}

template <typename HashCode, typename T>
void hash_decompose(HashCode& code, const vector<T>& v) {
  detail::hash_sized_container(code, v);
}

template <typename HashCode>
void hash_decompose(HashCode& code, const string& s) {
  detail::hash_sized_container(code, s);
}

// TODO: similar overloads for deque, list, set, map, multiset, multimap.
// Unordered containers are omitted as discussed in N3980. C-style arrays
// are omitted because they seem unlikely to be useful, and it's not entirely
// clear whether the size should be hashed.

// I've chosen to treat std::array as a container rather than a
// tuple-like type, meaning that the hash includes the size.
template <typename HashCode, typename T, size_t N>
void hash_decompose(HashCode& code, const array<T, N>& a) {
  detail::hash_sized_container(code, a);
}

template <typename HashCode, typename T>
void hash_decompose(HashCode& code, const forward_list<T>& l) {
  // We traverse the list manually rather than calling hash_combine_range,
  // so that we can compute the size without a second traversal. As with
  // hash_sized_container, we use size_t rather than the container's
  // size_type, for cross-container consistency.
  size_t size = 0;
  for (const T& t : l) {
    hash_combine(code, t);
    ++size;
  }
  hash_combine(code, size);
}

template <typename HashCode, typename T, typename D>
void hash_decompose(HashCode& code, const unique_ptr<T,D>& ptr) {
  hash_combine(code, ptr.get());
}

template <typename HashCode, typename T, typename U>
void hash_decompose(HashCode& code, const pair<T,U>& p) {
  hash_combine(code, p.first, p.second);
}

namespace detail {
template <typename HashCode, typename Tuple, size_t... Is>
void hash_tuple(HashCode& code, const Tuple& t, index_sequence<Is...>) {
  hash_combine(code, get<Is>(t)...);
}
}  // namespace detail

template <typename HashCode, typename... Ts>
void hash_decompose(HashCode& code, const tuple<Ts...>& t) {
  return detail::hash_tuple(code, t, make_index_sequence<sizeof...(Ts)>());
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

char* adl_pointer_from(string::iterator i) { return &*i; }

template <>
struct is_contiguous_iterator<string::const_iterator>
    : public true_type {};

const char* adl_pointer_from(string::const_iterator i) { return &*i; }

}  // namespace std

#endif   // HASHING_DEMO_STD_IMPL_H
