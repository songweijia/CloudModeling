#include <stdlib.h>
#include "cache_size.hpp"
#include "seq_thp.hpp"
#include "util.hpp"

#define NUM_ITER_PER_DP         (5)
#define BUFFER_ALIGNMENT        (4096)

static int32_t binary_search (
  const uint32_t seed_kb,
  const double target_thp,
  const bool is_write,
  const int32_t search_depth,
  uint32_t *output) {


  uint32_t lb = 0ul; // cache size lower bound in KB
  uint32_t ub = seed_kb << 2; // cache size upperbound in KB, 4 times of seed_kb.
  uint32_t pivot = seed_kb;
  double thps[NUM_ITER_PER_DP];
  void *buf;

  int32_t ret = posix_memalign(&buf, BUFFER_ALIGNMENT, ((size_t)ub)<<10);
  RETURN_ON_ERROR(ret,"posix_memalign()");

  int loop = search_depth;

  while(loop --) {
    size_t buffer_size = ((size_t)pivot)<<10;
    ret = sequential_throughput(buf,buffer_size,NUM_ITER_PER_DP,thps,is_write);
    RETURN_ON_ERROR(ret,"sequential_throughput");
    double v = average(NUM_ITER_PER_DP,thps);
    if (v < target_thp)
      ub = pivot;
    else
      lb = pivot;
    if (pivot == (ub + lb) / 2) // no change
      break;
    else
      pivot = (ub + lb) / 2;
  }

  *output = pivot;
  
  return ret;
}

int eval_cache_size(
  const uint32_t cache_size_hint_KB,
  const double upper_thp_GBps,
  const double lower_thp_GBps,
  uint32_t * css,
  const int num_samples,
  const bool is_write,
  const int32_t search_depth) {

  int i,ret;

  boost_cpu();

  for(i=0;i<num_samples;i++) {
    double thp = lower_thp_GBps + (upper_thp_GBps - lower_thp_GBps) * (i + 1) / (num_samples + 1);
    ret = binary_search (cache_size_hint_KB, thp, is_write, search_depth, &css[i]);
    RETURN_ON_ERROR(ret,"binary_search");
  }

  return ret;
}
