#pragma once
#include <inttypes.h>
#include <map>
#include <optional>
#include <stdio.h>
#include <vector>
#include <tuple>
#include <string>
#include <ci/util.hpp>

namespace cacheinspector {
/**
 * microbenchmark: sequential_throughput measurement
 * @param buffer        - pointer to the buffer, if NULL, this will allocate one.
 * @param buffer_size   - buffer size
 * @param num_iter      - the number of iteration to run
 * @param results       - output parameter, a pointer to a double array with 
 *                        at least num_iter entries, each of which is a 
 *                        throughput in Bytes/cycle. Note: we use the cycle with
 *                        rdtsc instruction, which may be normalized.
 * @param counters      - output parameter for receiving linux perf counters.
 * @param timing        - timing mechanism, CLOCK_GETTIME | RDTSC | PERF_CPU_CYCLE
 * @param bytes_per_iter- how many bytes to measure for each data point,
 *                        default to 256MiB.
 * @param num_iter_warmup
 *                      - number of iterations for warm up, default to 2
 * @param buf_alignment - alignment for the buffer, default to 4K
 * @return 0 for success, other values for failure.
 */
extern volatile int32_t sequential_throughput(
        void* buffer,
        size_t buffer_size,
        uint32_t num_iter,
        double* results,
        std::optional<std::vector<std::map<std::string, long long>>>& counters,
        timing_mechanism_t timing = CLOCK_GETTIME,
        const uint32_t is_write = 0,
        const uint64_t bytes_per_iter = (1ull << 28),
        const uint64_t num_iter_warmup = 5,
        const size_t buf_alignment = (1ull << 12));
}//namespace cacheinspector
