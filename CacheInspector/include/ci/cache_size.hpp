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
 * @param buf - the caller can provide a buffer for test. This parameter is useful when running cache_size detector
 *        periodically to avoid reallocating memory buffer in size eval_cache_size(), which takes hundreds of
 *        milliseconds to warm up.
 * @param buf_size - the size of the 'buf' provided by caller. We suggest size 256 MiB.
 * @param warm_up_cpu - do we need to warm up the cpu before testing? In systems, especailly those CPU is managed by a
 *        'powersave' policy, the CPU cores are generally running at a frequency as low as possible. This will affect
 *        the performance results. Assuming we use CLOCK_GETTIME timing mechanism, if the CPU is running at 2.9GHz, the
 *        L1 throughput will only be 3/4 of that when CPU is running at 3.8GHz. What about using HW_CPU_CYCLES? Changing
 *        CPU frequency is problematic still because memory response time is not affected by CPU clock, and the
 *        evaluated speed for memory throughput is 31 percent faster with 2.9GHz than with 3.8GHz. Since our cache size
 *        evaluation mechanism relies heavily on stable CPU frequency, we suggest always boost the cpu to its highest
 *        frequecy before testing. This can be avoid if the caller is sure about a stable CPU frequency.
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
        const int32_t search_depth = 12,
        const uint32_t num_iter_per_sample = 5,
        const uint64_t num_bytes_per_iter = (1ull << 28),
        void* buf = nullptr,
        const uint64_t buf_size = 0,
        const bool warm_up_cpu = true);
}// namespace cacheinspector
