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

#ifndef HASHING_DEMO_PIMPL_H
#define HASHING_DEMO_PIMPL_H

#include <memory>

#include "type_erased_hash_code.h"

class Impl;
hashing::type_erased_hash_code hash_value(
    hashing::type_erased_hash_code hash_code, const Impl& impl);

class Pimpl {
  std::unique_ptr<Impl> impl_;

 public:
  Pimpl();
  ~Pimpl();

  template <typename HashCode>
  friend HashCode hash_value(HashCode hash_code, const Pimpl& pimpl) {
    hash_value(hashing::type_erased_hash_code(&hash_code), *pimpl.impl_);
    return std::move(hash_code);
  }
};

#endif  // HASHING_DEMO_PIMPL_H
