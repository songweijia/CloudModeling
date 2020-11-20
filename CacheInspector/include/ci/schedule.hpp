#pragma once
#include <inttypes.h>
#include <map>
#include <optional>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <ci/util.hpp>

namespace cacheinspector {

typedef struct {
    /* buffer size */
    uint64_t    buffer_size;
    /* enabling sequential throughput test or not */
    bool        enable_thp;
    /* the total data size for sequential test */
    uint64_t    total_data_size;
    /* the number of data points to collect per throughput test */
    uint64_t    thp_num_datapoints;
    /* enabling random read latency test or not */
    bool        enable_lat;
    /* the number of data points to collect per latency test */
    uint64_t    lat_num_datapoints;
} ci_schedule_entry_t;

/**
 * schedule format: [s1,s2,...,sn]
 */
using ci_schedule_t = std::vector<ci_schedule_entry_t>;

class ScheduleResultCollector {
public:
    /** 
     * collect results for sequential throughput schedule
     *
     * @param is_write          true for write throughput, false for read throughput
     * @param buffer_size       the buffer size
     * @param num_datapoints    the number of data points in the result
     * @param results           the evaluated throughput
     * @param lpcs              the perf counters
     */
    virtual void collect_throughput(const bool is_write,
                                    const uint64_t buffer_size,
                                    const uint64_t num_datapoints,
                                    const double results[],
                                    const std::optional<std::vector<std::map<std::string, long long>>>& lpcs) = 0;
    /**
     * collect results for latency schedule
     *
     * @param buffer_size       the buffer size
     * @param num_datapoints    the number of data points in the result
     * @param results           the evaluated throughput
     * @param lpcs              the perf counters
     */
    virtual void collect_latency(const uint64_t buffer_size,
                                 const uint64_t num_datapoints,
                                 const double results[],
                                 const std::optional<std::vector<std::map<std::string, long long>>>& lpcs) = 0;
};

/**
 * microbenchmark: get the sequential throughput cliff chart data with a schedule
 *
 * @param schedule  - the test plan, each element is a 4-tuple of the numbers
 *                    Note: the elements in the schedule has to be in ascending order of the working set sizes.
 * @param collector - collecting results.
 * @param timing    - timing mechanism, CLOCK_GETTIME | RDTSC | PERF_CPU_CYCLE
 * @param show_perf_counters    - showing the perf counters or not.
 *
 * @return 0 for success, error code for failures.
 */
extern volatile int32_t ci_schedule(
            const ci_schedule_t& schedule,
            ScheduleResultCollector& collector,
            timing_mechanism_t timing = CLOCK_GETTIME,
            bool show_perf_counters = false);
}//namespace cacheinspector
