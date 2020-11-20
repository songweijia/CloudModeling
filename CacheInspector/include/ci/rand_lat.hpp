#pragma once
#include <map>
#include <optional>
#include <vector>
#include <string>

/**
 * Evaluate the random read latency for a given working set size(buffer_size)
 *
 * @param buffer_size   Working set/buffer size in bytes
 * @param num_points    Number of data points
 * @param output        Result in nano seconds
 * @param counters      Perf counters
 * @param timing        timing mechanism
 *
 * @return 0 for success, other values for failure.
 */
int32_t random_latency(int64_t buffer_size, 
                       int num_points,
                       double* output,
                       std::optional<std::vector<std::map<std::string, long long>>>& counters,
                       timing_mechanism_t timing=CLOCK_GETTIME);
