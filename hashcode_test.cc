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
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "debug.h"
#include "farmhash.h"
#include "fnv1a.h"
#include "pimpl.h"
#include "std.h"

namespace {

// HashHelper::Hash acts as an extension point, allowing us to customize
// how a particular HashCode is constructed and invoked. The primary
// template assumes HashCode is default-constructible.
template <typename HashCode, typename T>
struct HashHelper {
  static typename HashCode::result_type Hash(const T& t) {
    using std_::hash_value;
    return typename HashCode::result_type(
        hash_value(HashCode{}, t));
  }
};

template <typename T>
struct HashHelper<hashing::farmhash, T> {
  static hashing::farmhash::result_type Hash(const T& t) {
    using std_::hash_value;
    hashing::farmhash::state_type state;
    return hashing::farmhash::result_type(
        hash_value(hashing::farmhash{&state}, t));
  }
};

template <typename HashCode>
class HashCodeTest : public ::testing::Test {
 public:
  template <typename T>
  typename HashCode::result_type Hash(const T& t) {
    return HashHelper<HashCode, T>::Hash(t);
  }

  template <typename Int>
  void HashCombineIntegralTypeImpl();
};

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
  EXPECT_EQ(this->Hash(NoOp{}), this->Hash(NoOp{}));

  EXPECT_EQ(this->Hash(NoOp{}), this->Hash(EmptyCombine{}));

  EXPECT_EQ(this->Hash(NoOp{}), this->Hash(EmptyCombineRange{}));
}

template <typename HashCode>
template <typename Int>
void HashCodeTest<HashCode>::HashCombineIntegralTypeImpl() {
  SCOPED_TRACE(std::string("with integral type ") + typeid(Int).name());
  using limits = std::numeric_limits<Int>;
  EXPECT_NE(this->Hash(NoOp{}), this->Hash(Int(0)));
  EXPECT_NE(this->Hash(NoOp{}), this->Hash(limits::max()));
  EXPECT_NE(this->Hash(NoOp{}), this->Hash(limits::min()));

  EXPECT_EQ(this->Hash(CombineIterative<Int>{}),
            this->Hash(CombineVariadic<Int>{}));
  EXPECT_EQ(this->Hash(CombineIterative<Int>{}),
            this->Hash(CombineRange<Int>{}));
}

TYPED_TEST_P(HashCodeTest, HashCombineIntegralType) {
  this->template HashCombineIntegralTypeImpl<int>();
  this->template HashCombineIntegralTypeImpl<unsigned int>();
  this->template HashCombineIntegralTypeImpl<char>();
  this->template HashCombineIntegralTypeImpl<unsigned char>();
  this->template HashCombineIntegralTypeImpl<long>();
  this->template HashCombineIntegralTypeImpl<unsigned long>();
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

  EXPECT_EQ(this->Hash(s1[0]), this->Hash(s2[0]));
  EXPECT_EQ(this->Hash(ArraySlice<StructWithPadding>{s1, s1 + kNumStructs}),
            this->Hash(ArraySlice<StructWithPadding>{s2, s2 + kNumStructs}));
}

struct EquivalentToPimpl {
  std::vector<int> v_ = {1, 2, 3};
  std::string s_ = "abc";

  template <typename HashCode>
  friend HashCode hash_value(HashCode hash_code, const EquivalentToPimpl& e) {
    return hash_combine(std::move(hash_code), e.v_, e.s_);
  }
};

TYPED_TEST_P(HashCodeTest, HashPimplType) {
  EXPECT_EQ(this->Hash(EquivalentToPimpl{}), this->Hash(Pimpl{}));
}

REGISTER_TYPED_TEST_CASE_P(HashCodeTest,
                           NoOpsAreEquivalent,
                           HashCombineIntegralType,
                           HashNonUniquelyRepresentedType,
                           HashPimplType);

using HashCodeTypes = ::testing::Types<
  hashing::farmhash, hashing::fnv1a, hashing::type_invariant_fnv1a,
  hashing::identity>;
INSTANTIATE_TYPED_TEST_CASE_P(My, HashCodeTest, HashCodeTypes);

}  // namespace
