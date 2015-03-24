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

template <typename T, typename U>
T implicit_cast(U&& t) {
  return std::forward<U>(t);
}

template <typename H>
class TestHashCode {
 public:
  void Run() {
    TestDefaultConstruction();
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

  bool Equal(result_type r1, result_type r2) {
    return r1 == r2;
  }

  bool NotEqual(result_type r1, result_type r2) {
    return r1 != r2;
  }

  void TestDefaultConstruction() {
    H h1, h2;
    assert(Equal(std::move(h1), std::move(h2)));
    assert(Equal(H(), H()));
  }

  void TestNoOpCombine() {
    assert(Equal(hash_combine(H()), H()));
    std::vector<int> v;
    assert(Equal(hash_combine_range(H(), v.begin(), v.end()), H()));
  }

  template <typename Int>
  void TestHashCombineIntegralType() {
    using limits = std::numeric_limits<Int>;
    assert(NotEqual(hash_combine(H(), Int(0)), H()));
    assert(NotEqual(hash_combine(H(), limits::max()), H()));
    assert(NotEqual(hash_combine(H(), limits::min()), H()));

    H h;
    for (int i = 0; i < 5; ++i) {
      h = hash_combine(std::move(h), Int(i));
    }
    assert(Equal(std::move(h),
                 hash_combine(H(), Int(0), Int(1), Int(2), Int(3), Int(4))));

    h = H();
    for (int i = 0; i < 10; ++i) {
      h = hash_combine(std::move(h), Int(i));
    }
    std::vector<Int> v(10);
    std::iota(v.begin(), v.end(), Int(0));
    assert(Equal(std::move(h),
                 hash_combine_range(H(), v.begin(), v.end())));
  }
};

int main(int argc, char* argv[]) {
  TestHashCode<hashing::fnv1a>().Run();
  TestHashCode<hashing::type_invariant_fnv1a>().Run();
  TestHashCode<hashing::farmhash>().Run();
  std::cout << "All tests passed" << std::endl;
}
