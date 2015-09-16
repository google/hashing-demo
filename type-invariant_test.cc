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

#include <set>
#include <vector>

#include "fnv1a.h"
#include "gtest/gtest.h"

// String class that uses string interning to represent the contents.
// This makes things like hashing and comparison extremely efficient.
class InternedString {
  static std::set<const std::string> intern_pool_;
  const std::string* str_;

 public:
  InternedString(const char* str)
      :str_(&*intern_pool_.emplace(str).first) {}

  const std::string& value() { return *str_; }

  // All InternedStrings with the same value share the same str_ pointer,
  // so for ordinary hashing purposes we can hash the pointer, which is
  // much more efficient than hashing the whole string.
  template <typename HashCode>
  friend HashCode hash_value(HashCode hash_code, InternedString i) {
    return hash_combine(std::move(hash_code), i.str_);
  }

  // However, if we're being hashed by a type-invariant hash algorithm,
  // we're preumably being compared to other types of strings, so we
  // need to hash the full string value.
  friend hashing::type_invariant_fnv1a hash_value(
      hashing::type_invariant_fnv1a hash_code, InternedString i) {
    return hash_combine(std::move(hash_code), *i.str_);
  }
};

std::set<const std::string> InternedString::intern_pool_{};

namespace std_ {

template <>
struct is_uniquely_represented<InternedString> : public true_type {};

}  // namespace std_


TEST(TypeInvariantTest, TestTypeInvariance) {
  std::vector<InternedString> interned = {"a", "b", "c"};
  std::vector<std::string> ordinary = {"a", "b", "c"};

  using std_::hash_value;
  EXPECT_NE(size_t(hash_value(hashing::fnv1a{}, interned)),
            size_t(hash_value(hashing::fnv1a{}, ordinary)));
  EXPECT_EQ(size_t(hash_value(hashing::type_invariant_fnv1a{}, interned)),
            size_t(hash_value(hashing::type_invariant_fnv1a{}, ordinary)));
}
