cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

add_library(benchmarks STATIC
    cache_size.cpp
    rand_lat.cpp
#    rr_lat.c
    seq_thp.cpp
    linux_perf_counters.cpp
)
target_include_directories(benchmarks PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)
