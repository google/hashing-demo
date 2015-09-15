cmake_minimum_required (VERSION 2.8.12)
project (hashing-demo CXX)

set(gtest_src_dir /usr/src/gtest CACHE PATH "Path to gtest source root")
set(benchmark_src_dir benchmark)

add_subdirectory(${gtest_src_dir} gtest EXCLUDE_FROM_ALL)
SET(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL
    "Enable testing of the benchmark library.")
add_subdirectory(${benchmark_src_dir} benchmark EXCLUDE_FROM_ALL)

# Disable tr1/tuple, which is not available in libc++
set_property(DIRECTORY ${gtest_src_dir} APPEND PROPERTY COMPILE_DEFINITIONS GTEST_HAS_TR1_TUPLE=0)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS GTEST_HAS_TR1_TUPLE=0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall")

enable_testing()

add_executable(hashcode_test hashcode_test.cc pimpl.cc)
target_link_libraries(hashcode_test gtest_main)
add_test(hashcode_test hashcode_test)

add_executable(std_test std_test.cc)
target_link_libraries(std_test gtest_main)
add_test(std_test std_test)

add_executable(farmhash_golden_test farmhash_golden_test.cc)
add_test(farmhash_golden_test farmhash_golden_test)

add_executable(benchmarks benchmarks.cc)
target_link_libraries(benchmarks benchmark)
