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

#include <cassert>
#include <string>

#include "gtest/gtest.h"

#include "debug.h"
#include "std.h"

struct Hashable {
  int i;

  friend bool operator==(const Hashable& lhs, const Hashable& rhs) {
    return lhs.i == rhs.i;
  }

  friend std_::hash_code hash_value(std_::hash_code h, const Hashable& s) {
    return hash_combine(std::move(h), s.i);
  }
};

TEST(StdTest, UnorderedSetBasicUsage) {
  std_::unordered_set<Hashable> set1;
  set1.insert(Hashable{1});
  EXPECT_TRUE(set1.find(Hashable{1}) != set1.end());

  std_::unordered_set<std::string> set2;
  set2.insert("foo");
  EXPECT_TRUE(set2.find("foo") != set2.end());
}

TEST(StdTest, HashFloat) {
  EXPECT_EQ((std_::hash<float>{}(+0.0f)),
            (std_::hash<float>{}(-0.0f)));
}

struct LegacyHashable {
  size_t s;
};

namespace std_ {

template<>
struct hash<LegacyHashable> {
  size_t operator()(const LegacyHashable& d) {
    return d.s;
  }
};

}  // namespace std_

TEST(StdTest, LegacyHashingStillWorks) {
  EXPECT_EQ(0, std_::hash<LegacyHashable>{}(LegacyHashable{0}));
}

template <typename T, typename = void>
struct is_hashable : public std::false_type {};

template <typename T>
struct is_hashable<T,
                   std_::void_t<decltype(std_::hash<T>{}(std::declval<T>()))>>
    : public std::true_type {};

struct NotHashable {};

// Test that std::hash's operator() SFINAEs correctly
static_assert(is_hashable<Hashable>::value, "");
static_assert(is_hashable<LegacyHashable>::value, "");
static_assert(!is_hashable<NotHashable>::value, "");
