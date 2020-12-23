cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
project(cacheinspector C CXX)

include(GNUInstallDirs)

# Version
set(ci_VERSION 1.0)
set(ci_build_VERSION 1.0.0)

# CFLAGS
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-O3 -march=native")
set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -Wall -ggdb -gdwarf-3")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall")

#config.h
set(USE_PERF_SCHED_SWITCH 0)
set(USE_PERF_CPU_CYCLES 1)
set(USE_PERF_BUS_CYCLES 0)
set(USE_PERF_REF_CYCLES 0)
set(USE_INTEL_CPU_CYCLES 1)
set(USE_INTEL_REF_CYCLES 0)
set(USE_INTEL_LLC_REFS 1)
set(USE_INTEL_LLC_MISSES 1)
set(USE_INTEL_SANDYBRIDGE_L1C_HITS 1)
set(USE_INTEL_CASCADELAKE_L1C_HITS 0)
set(USE_INTEL_SANDYBRIDGE_L2C_HITS 1)
set(USE_INTEL_CASCADELAKE_L2C_HITS 0)
set(USE_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK 0)
set(USE_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK 0)
set(USE_INTEL_SANDYBRIDGE_ICACHE_MISSES 0)

set(LOG_BINARY_SEARCH 0)

set(ENABLE_PERF_TRACEPOINT 0)
if (${USE_PERF_SCHED_SWITCH} EQUAL 1)
    set(ENABLE_PERF_TRACEPOINT 1)
endif()

set(USE_HUGEPAGE 1)

execute_process(COMMAND lscpu
                COMMAND grep -o "avx2\\s"
                RESULT_VARIABLE TEST_AVX2)
execute_process(COMMAND lscpu
                COMMAND grep -o "avx\\s"
                RESULT_VARIABLE TEST_AVX)
execute_process(COMMAND lscpu
                COMMAND grep -o "avx512"
                RESULT_VARIABLE TEST_AVX512)
execute_process(COMMAND lscpu
                COMMAND grep -o "asimd"
                RESULT_VARIABLE TEST_ASIMD)

if (${TEST_AVX512} EQUAL 0)
    set(HAS_AVX512 1)
endif()
if (${TEST_AVX2} EQUAL 0)
    set(HAS_AVX2 1)
endif()
if (${TEST_AVX} EQUAL 0)
    set(HAS_AVX 1)
endif()
if (${TEST_ASIMD} EQUAL 0)
    set(HAS_ASIMD 1)
endif()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/include/ci/config.h.in ${CMAKE_BINARY_DIR}/include/ci/config.h)

find_package(Threads)

if (${ENABLE_PERF_TRACEPOINT} EQUAL 1)
add_custom_target(ci-tracepoint-header
    COMMAND ${CMAKE_SOURCE_DIR}/src/generate-tracepoint-ids-header.sh
    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/src/generate-tracepoint-ids-header.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ci
)
endif()

add_library(ci STATIC
    src/seq_thp.cpp
    src/rand_lat.cpp
    src/schedule.cpp
    src/cache_size.cpp
    src/linux_perf_counters.cpp
    src/cache_info_shm.cpp
)
target_include_directories(ci PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
if (${ENABLE_PERF_TRACEPOINT} EQUAL 1)
    add_dependencies(ci ci-tracepoint-header)
endif()
set_target_properties(ci PROPERTIES
    SOVERSION ${ci_VERSION}
    VERSION ${ci_build_VERSION}
)
target_link_libraries(ci ${CMAKE_THREAD_LIBS_INIT})

add_executable(ci-bin src/ci.cpp)
target_include_directories(ci-bin PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
target_link_libraries(ci-bin ci rt)
set_target_properties(ci-bin PROPERTIES OUTPUT_NAME ci)
add_custom_command(TARGET ci-bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/sample.schedule
    ${CMAKE_CURRENT_BINARY_DIR}/
)

add_executable(example src/example_app.cpp)
target_include_directories(example PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
target_link_libraries(example ci rt)

# make install
install(TARGETS ci EXPORT ci
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/)
install(DIRECTORY
        ${CMAKE_SOURCE_DIR}/include/ci
        ${CMAKE_BINARY_DIR}/include/ci
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp")
install(TARGETS ci-bin EXPORT ci-bin
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/)
install(FILES ${CMAKE_SOURCE_DIR}/profile/profile.py
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
        DESTINATION ${CMAKE_INSTALL_BINDIR}/)
