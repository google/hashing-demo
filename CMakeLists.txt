# Tested with CXX=clang++-libc++

cmake_minimum_required (VERSION 2.8.11)
project (hashing-demo)

enable_testing()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

add_executable(hashcode_test hashcode_test.cc)
add_test(hashcode_test hashcode_test)

add_executable(fnv1a_test fnv1a_test.cc)
add_test(fnv1a_test fnv1a_test)

add_executable(farmhash_test farmhash_test.cc)
add_test(farmhash_test farmhash_test)

add_executable(std_test std_test.cc)
add_test(std_test std_test)
