#pragma once
#include <map>
#include <optional>
#include <vector>
#include <string>

/**
 * Evaluate the read latency (memory to register) for a given working set size(buffer_size).
 *
 * @param buffer        Caller provided buffer: if the buffer is null, a new buffer will be created and destroyed 
 *                      on finish. 
 * @param buffer_size   Working set/buffer size in bytes
 * @param num_points    Number of data points
 * @param output        Result in nano seconds
 * @param counters      Perf counters
 * @param timing        timing mechanism
 *
 * @return 0 for success, other values for failure.
 */
int32_t rand_latency(void* buffer,
                     int64_t buffer_size, 
                     int num_points,
                     double* output,
                     std::optional<std::vector<std::map<std::string, long long>>>& counters,
                     timing_mechanism_t timing=CLOCK_GETTIME);
