#Introduction

This is a sample implementation of a forthcoming C++ standard library
proposal. It is intended only for demonstration and evaluation purposes,
and is not suitable for use in production systems. This is not an official
Google product (experimental or otherwise), it is just code that happens
to be owned by Google.

The APIs in [std.h](std.h) and [std_impl.h](std_impl.h) are proposed for
standardization. [fnv1a.h](fnv1a.h) and [farmhash.h](farmhash.h) are example
implementations of particular algorithms using this framework, but are not
themselves proposed for standardization. [std_test.cc](std_test.cc) shows some
simple examples of the extension API for type owners, as well as the end-user
API (which is just `std::hash`).

The code uses some C++14 language and library features. It doesn't work with
libstdc++ as of version 4.8, but it does work with libc++ 3.4. A
[CMakeLists.txt](CMakeLists.txt) is included for building the tests with
[CMake](http://www.cmake.org/). Example usage:
```Shell
$ cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++-libc++ $SOURCE_DIR
$ make
$ make test
```
The tests depend on [GoogleTest](https://code.google.com/p/googletest/),
which must be provided separately. By default, CMake will expect to find
the GoogleTest source in `/usr/src/gtest` and the headers in a default
include path, but these can be configured with `-Dgtest_src_dir` and
`-DCMAKE_INCLUDE_PATH`, respectively. Similarly, the benchmarks depend on
[google/benchmark](https://github.com/google/benchmark). This is included
as a git submodule (run `git submodule init; git submodule update` to set
it up), or you can install the source distribution in another location,
and configure that location with `-Dbenchmark_src_dir`.

API documentation will be provided in the forthcoming paper.
