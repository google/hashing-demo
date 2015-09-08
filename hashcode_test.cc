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

template <typename HashCode, typename T>
typename HashCode::result_type Hash(const T& t) {
  using std_::hash_value;
  typename HashCode::state_type state;
  return typename HashCode::result_type(
      hash_value(HashCode{&state}, t));
}

template <typename HashCode>
bool Equivalent(HashCode h1, HashCode h2) {
  using result_type = typename HashCode::result_type;
  return result_type(std::move(h1)) == result_type(std::move(h2));
}

template <typename HashCode>
class HashCodeTest : public  ::testing::Test {};

TYPED_TEST_CASE_P(HashCodeTest);

// Hashable types
//
// These types exist simply to exercise various hash_value behaviors, so
// they are named by what their hash_value does.

struct NoOp {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, NoOp n) {
    return std::move(h);
  }
};

struct EmptyCombine {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, EmptyCombine e) {
    return hash_combine(std::move(h));
  }
};

struct EmptyCombineRange {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, EmptyCombineRange e) {
    int dummy;
    return hash_combine_range(std::move(h), &dummy, &dummy);
  }
};

template <typename Int>
struct CombineIterative {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, CombineIterative c) {
    for (int i = 0; i < 5; ++i) {
      h = hash_combine(std::move(h), Int(i));
    }
    return h;
  }
};

template <typename Int>
struct CombineVariadic {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, CombineVariadic c) {
    return hash_combine(std::move(h), Int(0), Int(1), Int(2), Int(3), Int(4));
  }
};

template <typename Int>
struct CombineRange {
  template <typename HashCode>
  friend HashCode hash_value(HashCode h, CombineRange c) {
    Int ints[] = {Int(0), Int(1), Int(2), Int(3), Int(4)};
    return hash_combine_range(std::move(h), std::begin(ints), std::end(ints));
  }
};

TYPED_TEST_P(HashCodeTest, NoOpsAreEquivalent) {
  EXPECT_EQ(Hash<TypeParam>(NoOp{}), Hash<TypeParam>(NoOp{}));

  EXPECT_EQ(Hash<TypeParam>(NoOp{}), Hash<TypeParam>(EmptyCombine{}));

  EXPECT_EQ(Hash<TypeParam>(NoOp{}), Hash<TypeParam>(EmptyCombineRange{}));
}

template <typename HashCode, typename Int>
void HashCombineIntegralTypeImpl() {
  SCOPED_TRACE(std::string("with integral type ") + typeid(Int).name());
  using limits = std::numeric_limits<Int>;
  EXPECT_NE(Hash<HashCode>(NoOp{}), Hash<HashCode>(Int(0)));
  EXPECT_NE(Hash<HashCode>(NoOp{}), Hash<HashCode>(limits::max()));
  EXPECT_NE(Hash<HashCode>(NoOp{}), Hash<HashCode>(limits::min()));

  EXPECT_EQ(Hash<HashCode>(CombineIterative<Int>{}),
            Hash<HashCode>(CombineVariadic<Int>{}));
  EXPECT_EQ(Hash<HashCode>(CombineIterative<Int>{}),
            Hash<HashCode>(CombineRange<Int>{}));
}

TYPED_TEST_P(HashCodeTest, HashCombineIntegralType) {
  HashCombineIntegralTypeImpl<TypeParam, int>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned int>();
  HashCombineIntegralTypeImpl<TypeParam, char>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned char>();
  HashCombineIntegralTypeImpl<TypeParam, long>();
  HashCombineIntegralTypeImpl<TypeParam, unsigned long>();
}

struct StructWithPadding {
  char c;
  int i;

  template <typename HashCode>
  friend HashCode hash_value(HashCode hash_code, const StructWithPadding& s) {
    return hash_combine(std::move(hash_code), s.c, s.i);
  }
};

static_assert(sizeof(StructWithPadding) > sizeof(char) + sizeof(int),
              "StructWithPadding doesn't have padding");
static_assert(std::is_trivially_constructible<StructWithPadding>::value, "");
static_assert(std::is_standard_layout<StructWithPadding>::value, "");

template <typename T>
struct ArraySlice {
  T* begin;
  T* end;

  template <typename HashCode>
  friend HashCode hash_value(HashCode hash_code, const ArraySlice& slice) {
    return hash_combine_range(std::move(hash_code), slice.begin, slice.end);
  }
};

TYPED_TEST_P(HashCodeTest, HashNonUniquelyRepresentedType) {
  // Create equal StructWithPadding objects that are known to have non-equal
  // padding bytes.
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

  EXPECT_EQ(Hash<TypeParam>(s1[0]), Hash<TypeParam>(s2[0]));
  EXPECT_EQ(
      Hash<TypeParam>(ArraySlice<StructWithPadding>{s1, s1 + kNumStructs}),
      Hash<TypeParam>(ArraySlice<StructWithPadding>{s2, s2 + kNumStructs}));
}

REGISTER_TYPED_TEST_CASE_P(HashCodeTest,
                           NoOpsAreEquivalent,
                           HashCombineIntegralType,
                           HashNonUniquelyRepresentedType);

using HashCodeTypes = ::testing::Types<
  hashing::farmhash, hashing::fnv1a, hashing::type_invariant_fnv1a,
  hashing::identity>;
INSTANTIATE_TYPED_TEST_CASE_P(My, HashCodeTest, HashCodeTypes);

}  // namespace
