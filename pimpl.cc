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

#include "pimpl.h"

#include <string>
#include <vector>

class Impl {
  std::vector<int> v_ = {1, 2, 3};
  std::string s_ = "abc";

 public:
  Impl() {}

  friend hashing::type_erased_hash_code hash_value(
      hashing::type_erased_hash_code hash_code, const Impl& impl);
};

hashing::type_erased_hash_code hash_value(
    hashing::type_erased_hash_code hash_code, const Impl& impl) {
  return hash_combine(std::move(hash_code), impl.v_, impl.s_);
}

Pimpl::Pimpl() :impl_(std::make_unique<Impl>()) {}

Pimpl::~Pimpl() {}
