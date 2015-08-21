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

namespace {

template <typename HashCode>
bool Equivalent(HashCode& h1, HashCode& h2) {
  using result_type = typename HashCode::result_type;
  return result_type(h1) == result_type(h2);
}

template <typename HashCode>
class HashCodeTest : public  ::testing::Test {};

TYPED_TEST_CASE_P(HashCodeTest);

TYPED_TEST_P(HashCodeTest, InitialStatesAreEqual) {
  TypeParam h1, h2;
  EXPECT_TRUE(Equivalent(h1, h2));
}

TYPED_TEST_P(HashCodeTest, EmptyCombineIsNoOp) {
  {
    TypeParam h1, h2;
    hash_combine(h1);
    EXPECT_TRUE(Equivalent(h1, h2));
  }

  {
    std::vector<int> v;
    TypeParam h1, h2;
    hash_combine_range(h1, v.begin(), v.end());
    EXPECT_TRUE(Equivalent(h1, h2));
  }
}

template <typename HashCode, typename Int>
void HashCombineIntegralTypeImpl() {
  SCOPED_TRACE(std::string("with integral type ") + typeid(Int).name());
  using limits = std::numeric_limits<Int>;
  HashCode empty;
  {
    HashCode h;
    hash_combine(h, Int(0));
    EXPECT_FALSE(Equivalent(empty, h));
  }
  {
    HashCode h;
    hash_combine(h, limits::max());
    EXPECT_FALSE(Equivalent(empty, h));
  }
  {
    HashCode h;
    hash_combine(h, limits::min());
    EXPECT_FALSE(Equivalent(empty, h));
  }

  {
    HashCode h1;
    for (int i = 0; i < 5; ++i) {
      hash_combine(h1, Int(i));
    }
    HashCode h2;
    hash_combine(h2, Int(0), Int(1), Int(2), Int(3), Int(4));
    EXPECT_TRUE(Equivalent(h1, h2));
  }

  {
    HashCode h1;
    for (int i = 0; i < 10; ++i) {
      hash_combine(h1, Int(i));
    }
    std::vector<Int> v(10);
    std::iota(v.begin(), v.end(), Int(0));
    HashCode h2;
    hash_combine_range(h2, v.begin(), v.end());
    EXPECT_TRUE(Equivalent(h1, h2));
  }
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
void hash_decompose(HashCode& hash_code, const StructWithPadding& s) {
  hash_combine(hash_code, s.c, s.i);
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

  {
    TypeParam h1;
    hash_combine(h1, s1[0], s1[1]);
    TypeParam h2;
    hash_combine(h2, s2[0], s2[1]);
    EXPECT_TRUE(Equivalent(h1, h2));
  }

  {
    TypeParam h1;
    hash_combine_range(h1, s1, s1 + kNumStructs);
    TypeParam h2;
    hash_combine_range(h2, s2, s2 + kNumStructs);
    EXPECT_TRUE(Equivalent(h1, h2));
  }
}

REGISTER_TYPED_TEST_CASE_P(HashCodeTest,
                           InitialStatesAreEqual, EmptyCombineIsNoOp,
                           HashCombineIntegralType,
                           HashNonUniquelyRepresentedType);

using HashCodeTypes = ::testing::Types<
  hashing::farmhash, hashing::fnv1a, hashing::type_invariant_fnv1a,
  hashing::identity>;
INSTANTIATE_TYPED_TEST_CASE_P(My, HashCodeTest, HashCodeTypes);

}  // namespace
