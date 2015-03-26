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

// Type-parameterized hash algorithm tests.

#include <cassert>
#include <iostream>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "farmhash.h"
#include "fnv1a.h"
#include "std.h"

namespace {

template <typename HashCode>
struct StateAnd {
  StateAnd() : hash_code(&state) {}
  typename HashCode::state_type state;
  HashCode hash_code;
};

template <typename HashCode>
bool Equivalent(HashCode h1, HashCode h2) {
  using result_type = typename HashCode::result_type;
  return result_type(std::move(h1)) == result_type(std::move(h2));
}

template <typename HashCode>
class HashCodeTest : public  ::testing::Test {};

TYPED_TEST_CASE_P(HashCodeTest);

TYPED_TEST_P(HashCodeTest, InitialStatesAreEqual) {
  EXPECT_TRUE(Equivalent(StateAnd<TypeParam>().hash_code,
                         StateAnd<TypeParam>().hash_code));
}

TYPED_TEST_P(HashCodeTest, EmptyCombineIsNoOp) {
  EXPECT_TRUE(Equivalent(hash_combine(StateAnd<TypeParam>().hash_code),
                         StateAnd<TypeParam>().hash_code));
  std::vector<int> v;
  EXPECT_TRUE(Equivalent(hash_combine_range(StateAnd<TypeParam>().hash_code,
                                            v.begin(), v.end()),
                         StateAnd<TypeParam>().hash_code));
}

template <typename HashCode, typename Int>
void HashCombineIntegralTypeImpl() {
  SCOPED_TRACE(std::string("with integral type ") + typeid(Int).name());
  using limits = std::numeric_limits<Int>;
  EXPECT_FALSE(Equivalent(
      hash_combine(StateAnd<HashCode>().hash_code, Int(0)),
      StateAnd<HashCode>().hash_code));
  EXPECT_FALSE(Equivalent(
      hash_combine(StateAnd<HashCode>().hash_code, limits::max()),
      StateAnd<HashCode>().hash_code));
  EXPECT_FALSE(Equivalent(
      hash_combine(StateAnd<HashCode>().hash_code, limits::min()),
      StateAnd<HashCode>().hash_code));

  StateAnd<HashCode> s;
  for (int i = 0; i < 5; ++i) {
    s.hash_code = hash_combine(std::move(s.hash_code), Int(i));
  }
  EXPECT_TRUE(Equivalent(
      std::move(s.hash_code),
      hash_combine(StateAnd<HashCode>().hash_code,
                   Int(0), Int(1), Int(2), Int(3), Int(4))));

  StateAnd<HashCode> s2;
  for (int i = 0; i < 10; ++i) {
    s2.hash_code = hash_combine(std::move(s2.hash_code), Int(i));
  }
  std::vector<Int> v(10);
  std::iota(v.begin(), v.end(), Int(0));
  EXPECT_TRUE(Equivalent(
      std::move(s2.hash_code),
      hash_combine_range(StateAnd<HashCode>().hash_code,
                         v.begin(), v.end())));
}

TYPED_TEST_P(HashCodeTest, HashCombineIntegralType) {
  HashCombineIntegralTypeImpl<TypeParam, int>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned int>();
  HashCombineIntegralTypeImpl<TypeParam, char>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned char>();
  HashCombineIntegralTypeImpl<TypeParam, long>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned long>();
}

REGISTER_TYPED_TEST_CASE_P(HashCodeTest,
                           InitialStatesAreEqual, EmptyCombineIsNoOp,
                           HashCombineIntegralType);

using HashCodeTypes = ::testing::Types<
  hashing::farmhash, hashing::fnv1a, hashing::type_invariant_fnv1a>;
INSTANTIATE_TYPED_TEST_CASE_P(My, HashCodeTest, HashCodeTypes);

}  // namespace
