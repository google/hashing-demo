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

TEST(StdTest, UnorderedSetBasicUsage) {
  std_::unordered_set<int> s1;
  s1.insert(1);
  EXPECT_TRUE(s1.find(1) != s1.end());

  std_::unordered_set<std::string> s2;
  s2.insert("foo");
  EXPECT_TRUE(s2.find("foo") != s2.end());
}

TEST(StdTest, HashFloat) {
  EXPECT_EQ((std_::hash<float>{}(+0.0f)),
            (std_::hash<float>{}(-0.0f)));
}

struct Dummy {
  size_t s;
};

namespace std_ {

template<>
struct hash<Dummy> {
  size_t operator()(const Dummy& d) {
    return d.s;
  }
};

}  // namespace std_

TEST(StdTest, LegacyHashingStillWorks) {
  EXPECT_EQ(0, std_::hash<Dummy>{}(Dummy{0}));
}
