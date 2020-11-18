#pragma once
#include <inttypes.h>
#include <map>
#include <optional>
#include <stdio.h>
#include <vector>
#include <tuple>
#include <string>

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
        const uint32_t is_write = 0,
        const uint64_t bytes_per_iter = (1ull << 28),
        const uint64_t num_iter_warmup = 5,
        const size_t buf_alignment = (1ull << 12));

using sequential_throughput_test_schedule_t = std::vector<std::tuple<uint64_t,uint64_t,uint64_t>>;
using sequential_throughput_test_result_t = std::vector<std::tuple<double,double,double,double>>;

extern const sequential_throughput_test_schedule_t default_sequential_throughput_test_schedule;

/**
 * microbenchmark: get the sequential throughput cliff chart data
 *
 * @param schedule  - the test plan, each element is a 3-tuple of the numbers
 *                    0) the working set (buffer) size in Bytes
 *                    1) the total access size in Bytes
 *                    2) the number of samples/iterations to run for this working set size.
 *                    Note: the elements in the schedule has to be in ascending order of the working set sizes.
 * @param result    - the result, each element is a 4-tuple of the numbers
 *                    0) the average of the read throughput
 *                    1) the standard deviation of the read throughput deviation.
 *                    2) the average of the write throughput
 *                    3) the standard deviation of the write throughput deviation.
 * @return 0 for success, other values for failure.
 */
extern volatile int32_t sequential_throughput_cliffs(
            const sequential_throughput_test_schedule_t& schedule,
            sequential_throughput_test_result_t& result);

}//namespace cacheinspector
