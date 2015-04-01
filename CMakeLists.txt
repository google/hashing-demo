cmake_minimum_required (VERSION 2.8.12)
project (hashing-demo CXX)

set(gtest_src_dir /usr/src/gtest CACHE PATH "Path to gtest source root")
set(benchmark_src_dir benchmark)

add_subdirectory(${gtest_src_dir} gtest)
add_subdirectory(${benchmark_src_dir} benchmark)

# Disable tr1/tuple, which is not available in libc++
set_property(DIRECTORY ${gtest_src_dir} APPEND PROPERTY COMPILE_DEFINITIONS GTEST_HAS_TR1_TUPLE=0)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS GTEST_HAS_TR1_TUPLE=0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall")

enable_testing()

function(add_gtest test_name)
  add_executable(${test_name} "${test_name}.cc")
  target_link_libraries(${test_name} gtest_main)
  add_test(${test_name} ${test_name})
endfunction(add_gtest)

add_gtest(hashcode_test)
add_gtest(std_test)

add_executable(farmhash_golden_test farmhash_golden_test.cc)
add_test(farmhash_golden_test farmhash_golden_test)

add_executable(benchmarks benchmarks.cc)
target_link_libraries(benchmarks benchmark)

# Add necessary dependencies (TODO: drop this if I can get upstream
# to do it automatically).
find_package(Threads REQUIRED)
target_link_libraries(benchmarks ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(benchmarks PRIVATE "${benchmark_src_dir}/include")

