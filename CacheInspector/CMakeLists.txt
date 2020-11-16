cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
project(cacheinspector C CXX)

# Version
set(cacheinspector 0.9)
set(cacheinspector_build_version 0.9.1)

# CFLAGS
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_CXX_FLAGS "-fPIC -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -ggdb -gdwarf-3")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

#config.h
set(TIMING_WITH_CLOCK_GETTIME 1)
set(TIMING_WITH_RDTSC 0)
set(TIMING_WITH_CPU_CYCLE 0)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

add_subdirectory(benchmarks)

add_executable(ci ci.cpp)
target_include_directories(ci PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/benchmarks>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)
target_link_libraries(ci benchmarks)
