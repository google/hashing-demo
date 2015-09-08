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
#include <unordered_set>

#include "gtest/gtest.h"

#include "debug.h"
#include "std.h"

// TODO(gromer): re-enable this in terms of std::hash
#if 0
TEST(StdTest, UnorderedSetBasicUsage) {
  std::unordered_set<int, std::hasher<int>> s1;
  s1.insert(1);
  EXPECT_TRUE(s1.find(1) != s1.end());

  std::unordered_set<std::string, std::hasher<std::string>> s2;
  s2.insert("foo");
  EXPECT_TRUE(s2.find("foo") != s2.end());
}

TEST(StdTest, HashFloat) {
  EXPECT_EQ((std::hasher<float, hashing::identity>{}(+0.0f)),
            (std::hasher<float, hashing::identity>{}(-0.0f)));
}
#endif
