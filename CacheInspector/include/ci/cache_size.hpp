#pragma once
#include <inttypes.h>
#include <ci/util.hpp>

namespace cacheinspector {
/**
 * evaluate the cache size for a given instance
 * @param cache_size_hint_KiB
 * @param upper_thp - if we are measuring L2 cache, this is going to be 
 *        the pre-evaluated throughput for L2 cache. The unit of the throughput
 *        is decided by timing configuration used (see config.mk):
 *        1) For 'cpu_cycles', the throughput is in bytes per cpu cycle
 *        2) For 'rdtsc', the throughput is in bytes per tsc cycle
 *        3) For 'clock_gettime', the throughput is in GiB per second
 * @param lower_thp - if we are measuring L3 cache, this is going to be
 *        the pre-evaluated throughput of L3 cache (or memory if no L3 cache)
 * @param css - the output parameter, pointing to an array of uint32_t with
 *        'num_samples' entries. They are in KiB.
 * @param num_samples - the number of evaluated cache sizes. Multiple cache size
 *        estimations give the distribution of the cache size. Let's say the 
 *        throughput of L2 and L3 cache are 20/30GiB/s respectively. the 10GiB/s
 *        gap will be interpolated by 'num_samples' data points. The i-th point
 *        shows where estimation of cache size will be to get this throughput:
 *        20 + 10*i/(num_samples+1) GiB/s throughput. Defaulted to 1.
 * @param is_write - true, if upper_thp/lower_thp is write throughput
 *        False, otherwise.
 * @param timing - timing mechanism: CLOCK_GETTIME | RDTSC | PERF_CPU_CYCLE | HW_CPU_CYCLE
 * @param search_depth - how many iterations through the binary search.
 * @param num_iter_per_sample - how many iteration for each data points we will
 *        run.
 * @param num_bytes_per_iter - how many bytes to scan, this will be passed to
 *        sequential_throughput() in bytes_per_iter. Default to 256MiB.
 * @return 0 for success, otherwise failure
 */
extern int eval_cache_size(
        const uint32_t cache_size_hint_KiB,
        const double upper_thp,
        const double lower_thp,
        uint32_t* css,
        const int num_samples = 1,
        const bool is_write = true,
        const timing_mechanism_t timing = CLOCK_GETTIME,
        const int32_t search_depth = 10,
        const uint32_t num_iter_per_sample = 5,
        const uint64_t num_bytes_per_iter = (1ull << 28));
}// namespace cacheinspector
