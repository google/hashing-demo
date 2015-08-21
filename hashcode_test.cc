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

#include "debug.h"
#include "farmhash.h"
#include "fnv1a.h"
#include "std.h"
#include "type_invariant.h"

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
class HashCodeTest : public ::testing::Test {};

TYPED_TEST_CASE_P(HashCodeTest);

TYPED_TEST_P(HashCodeTest, InitialStatesAreEqual) {
  EXPECT_TRUE(Equivalent(StateAnd<TypeParam>().hash_code,
                         StateAnd<TypeParam>().hash_code));
}

TYPED_TEST_P(HashCodeTest, EmptyCombineIsNoOp) {
  using std::hash_combine;
  EXPECT_TRUE(Equivalent(hash_combine(StateAnd<TypeParam>().hash_code),
                         StateAnd<TypeParam>().hash_code));
  std::vector<int> v;
  EXPECT_TRUE(Equivalent(
      hash_combine(StateAnd<TypeParam>().hash_code,
                   std::make_iterator_range(v.begin(), v.end())),
      StateAnd<TypeParam>().hash_code));
}

template <typename HashCode, typename Int>
void HashCombineIntegralTypeImpl() {
  using std::hash_combine;
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
      hash_combine(StateAnd<HashCode>().hash_code,
                   std::make_iterator_range(v.begin(), v.end()))));
}

TYPED_TEST_P(HashCodeTest, HashCombineIntegralType) {
  HashCombineIntegralTypeImpl<TypeParam, int>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned int>();
  HashCombineIntegralTypeImpl<TypeParam, char>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned char>();
  HashCombineIntegralTypeImpl<TypeParam, long>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned long>();
}

namespace test_namespace {

struct StructWithPadding {
  char c;
  int i;
};

static_assert(sizeof(StructWithPadding) > sizeof(char) + sizeof(int),
              "StructWithPadding doesn't have padding");
static_assert(std::is_trivially_constructible<StructWithPadding>::value, "");
static_assert(std::is_standard_layout<StructWithPadding>::value, "");

template <typename HashCode>
HashCode hash_combine(HashCode hash_code, const StructWithPadding& s) {
  using std::hash_combine;
  return hash_combine(std::move(hash_code), s.c, s.i);
}

}  // namespace test_namespace

TYPED_TEST_P(HashCodeTest, HashNonUniquelyRepresentedType) {
  // Create equal StructWithPadding objects that are known to have non-equal
  // padding bytes.
  using test_namespace::StructWithPadding;
  static const size_t kNumStructs = 10;
  unsigned char buffer1[kNumStructs * sizeof(StructWithPadding)];
  std::memset(buffer1, 0, sizeof(buffer1));
  auto* s1 = reinterpret_cast<StructWithPadding*>(buffer1);

  unsigned char buffer2[kNumStructs * sizeof(StructWithPadding)];
  std::memset(buffer1, 255, sizeof(buffer2));
  auto* s2 = reinterpret_cast<StructWithPadding*>(buffer2);
  for (int i = 0; i < kNumStructs; ++i) {
    SCOPED_TRACE(i);
    s1[i].c = s2[i].c = '0' + i;
    s1[i].i = s2[i].i = i;
    ASSERT_FALSE(memcmp(buffer1 + i * sizeof(StructWithPadding),
                        buffer2 + 1 * sizeof(StructWithPadding),
                        sizeof(StructWithPadding)) == 0)
        << "Bug in test code: objects do not have unequal"
        << " object representations";
  }

  using std::hash_combine;
  EXPECT_TRUE(Equivalent(
      hash_combine(StateAnd<TypeParam>().hash_code, s1[0], s1[1]),
      hash_combine(StateAnd<TypeParam>().hash_code, s2[0], s1[1])));

  EXPECT_TRUE(Equivalent(
      hash_combine(StateAnd<TypeParam>().hash_code,
                   std::make_iterator_range(s1, s1 + kNumStructs)),
      hash_combine(StateAnd<TypeParam>().hash_code,
                   std::make_iterator_range(s2, s2 + kNumStructs))));
}

REGISTER_TYPED_TEST_CASE_P(HashCodeTest,
                           InitialStatesAreEqual, EmptyCombineIsNoOp,
                           HashCombineIntegralType,
                           HashNonUniquelyRepresentedType);

using HashCodeTypes = ::testing::Types<
  hashing::farmhash, hashing::fnv1a, hashing::identity,
  hashing::type_invariant_hash<hashing::farmhash>,
  hashing::type_invariant_hash<hashing::fnv1a>>;
INSTANTIATE_TYPED_TEST_CASE_P(My, HashCodeTest, HashCodeTypes);

template <typename HashCode>
class InvariantHashCodeTest : public ::testing::Test {};

TYPED_TEST_CASE_P(InvariantHashCodeTest);

// Struct that is uniquely represented, but has a hash representation that's
// not identical to its object representation (this can happen e.g. if you
// want to match the hash representation of a different type).
struct CustomHashRep {
  short value;
};

template <typename HashCode>
HashCode hash_combine(HashCode hash_code, const CustomHashRep& c) {
  using std::hash_combine;
  return hash_combine(std::move(hash_code), static_cast<long>(c.value));
}

} // namespace

namespace std {
template<> struct is_uniquely_represented<::CustomHashRep> : true_type {};
}

namespace {

TYPED_TEST_P(InvariantHashCodeTest, HashesIndividualValues) {
  CustomHashRep structs[3] = {{1}, {2}, {3}};
  long equivalent[3] = {1, 2, 3};

  using std::hash_combine;
  EXPECT_TRUE(Equivalent(
      hash_combine(StateAnd<TypeParam>().hash_code,
                   std::make_iterator_range(structs, structs + 3)),
      hash_combine(StateAnd<TypeParam>().hash_code,
                   std::make_iterator_range(equivalent, equivalent + 3))));
}

REGISTER_TYPED_TEST_CASE_P(InvariantHashCodeTest,
                           HashesIndividualValues);

using InvariantHashCodeTypes = ::testing::Types<
  hashing::identity, hashing::type_invariant_hash<hashing::farmhash>,
  hashing::type_invariant_hash<hashing::fnv1a>>;
INSTANTIATE_TYPED_TEST_CASE_P(My, InvariantHashCodeTest,
                              InvariantHashCodeTypes);

}  // namespace
