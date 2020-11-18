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
set(CMAKE_C_FLAGS "-O3 -march=native")
set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -ggdb -gdwarf-3")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

#config.h
set(TIMING_WITH_CLOCK_GETTIME 1)
set(TIMING_WITH_RDTSC 0)
set(TIMING_WITH_CPU_CYCLE 0)
execute_process(COMMAND lscpu
                COMMAND grep -o "avx2\\s"
                RESULT_VARIABLE TEST_AVX2)
execute_process(COMMAND lscpu
                COMMAND grep -o "avx\\s"
                RESULT_VARIABLE TEST_AVX)
execute_process(COMMAND lscpu
                COMMAND grep -o "avx512"
                RESULT_VARIABLE TEST_AVX512)

if (${TEST_AVX512} EQUAL 0)
    set(HAS_AVX512 1)
elseif (${TEST_AVX2} EQUAL 0)
    set(HAS_AVX2 1)
elseif (${TEST_AVX} EQUAL 0)
    set(HAS_AVX 1)
endif()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/ci/config.h.in ${CMAKE_BINARY_DIR}/include/ci/config.h)

add_library(ci STATIC
    src/seq_thp.cpp
    src/cache_size.cpp
    src/rand_lat.cpp
#   src/rr_lat.c
    src/linux_perf_counters.cpp
)
target_include_directories(ci PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

add_executable(ci-bin src/ci.cpp)
target_include_directories(ci-bin PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
target_link_libraries(ci-bin ci)
set_target_properties(ci-bin PROPERTIES OUTPUT_NAME ci)
