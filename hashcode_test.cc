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

#include "farmhash.h"
#include "fnv1a.h"
#include "std.h"

template <typename H>
class TestHashCode {
 public:
  void Run() {
    TestInitialConstruction();
    TestNoOpCombine();
    TestHashCombineIntegralType<int>();
    TestHashCombineIntegralType<unsigned int>();
    TestHashCombineIntegralType<char>();
    TestHashCombineIntegralType<unsigned char>();
    TestHashCombineIntegralType<long>();
    TestHashCombineIntegralType<unsigned long>();
  }

 private:
  using result_type = typename H::result_type;
  using state_type = typename H::state_type;

  struct StateAndH {
    StateAndH() : h(&state) {}
    state_type state;
    H h;
  };

  bool Equal(H h1, H h2) {
    return result_type(std::move(h1)) == result_type(std::move(h2));
  }

  bool NotEqual(H h1, H h2) {
    return result_type(std::move(h1)) != result_type(std::move(h2));
  }

  void TestInitialConstruction() {
    assert(Equal(StateAndH().h, StateAndH().h));
  }

  void TestNoOpCombine() {
    assert(Equal(hash_combine(StateAndH().h), StateAndH().h));
    std::vector<int> v;
    assert(Equal(hash_combine_range(StateAndH().h, v.begin(), v.end()),
                 StateAndH().h));
  }

  template <typename Int>
  void TestHashCombineIntegralType() {
    using limits = std::numeric_limits<Int>;
    assert(NotEqual(hash_combine(StateAndH().h, Int(0)), StateAndH().h));
    assert(NotEqual(hash_combine(StateAndH().h, limits::max()), StateAndH().h));
    assert(NotEqual(hash_combine(StateAndH().h, limits::min()), StateAndH().h));

    StateAndH s;
    for (int i = 0; i < 5; ++i) {
      s.h = hash_combine(std::move(s.h), Int(i));
    }
    assert(Equal(std::move(s.h),
                 hash_combine(StateAndH().h,
                              Int(0), Int(1), Int(2), Int(3), Int(4))));

    StateAndH s2;
    for (int i = 0; i < 10; ++i) {
      s2.h = hash_combine(std::move(s2.h), Int(i));
    }
    std::vector<Int> v(10);
    std::iota(v.begin(), v.end(), Int(0));
    assert(Equal(std::move(s2.h),
                 hash_combine_range(StateAndH().h, v.begin(), v.end())));
  }
};

int main(int argc, char* argv[]) {
  TestHashCode<hashing::fnv1a>().Run();
  TestHashCode<hashing::type_invariant_fnv1a>().Run();
  TestHashCode<hashing::farmhash>().Run();
  std::cout << "All tests passed" << std::endl;
}
