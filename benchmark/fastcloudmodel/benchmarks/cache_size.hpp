#ifndef _CACHE_SIZE_HPP_
#define _CACHE_SIZE_HPP_

#include <inttypes.h>

/**
 * evaluate the cache size for a given instance
 * @param cache_size_hint_KB
 * @param upper_thp_GBps - if we are measuring L2 cache, this is going to be 
 *        the pre-evaluated throughput for L2 cache
 * @param lower_thp_GBps - if we are measuring L3 cache, this is going to be
 *        the pre-evaluated throughput of L3 cache (or memory if no L3 cache)
 * @param css - the output parameter, pointing to an array of uint32_t with
 *        'num_samples' entries. They are the values in KB.
 * @param num_samples - the number of evaluated cache sizes. Multiple cache size
 *        estimations give the distribution of the cache size. Let's say the 
 *        throughput of L2 and L3 cache are 20/30GBps respectively. the 10GBps
 *        gap will be interpolated by 'num_samples' data points. The i-th point
 *        shows where estimation of cache size will be to get this throughput:
 *        20 + 10*i/(num_samples+1) GBps throughput. Defaulted to 1.
 * @param is_write - true, if upper_thp_GBps/lower_thp_GBps is write throughput
 *                   False, otherwise.
 * @param search_depth - how many iterations through the binary search.
 * @return 0 for success, otherwise failure
 */
extern int eval_cache_size(
  const uint32_t cache_size_hint_KB,
  const double upper_thp_GBps,
  const double lower_thp_GBps,
  uint32_t * css,
  const int num_samples = 1,
  const bool is_write = true,
  const int32_t search_depth = 10);

#endif//__CACHE_SIZE_HPP_
