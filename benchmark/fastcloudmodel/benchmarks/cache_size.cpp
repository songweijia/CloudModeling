#include <stdlib.h>
#include "cache_size.hpp"
#include "seq_thp.hpp"
#include "util.hpp"

#define NUM_ITER_PER_DP         (5)
#define BUFFER_ALIGNMENT        (4096)
#define MEM_ALLOCATION		(256ull << 20)

static int32_t binary_search (
  const uint32_t seed_kb,
  const double target_thp,
  const bool is_write,
  const int32_t search_depth,
  void * workspace, // a memory workspace with MEM_ALLOCATION 256MB.
  uint32_t *output) {

  uint32_t lb = 0ul; // cache size lower bound in KB
  uint32_t ub = 0ul; // cache size upperbound in KB
  uint32_t pivot = seed_kb;
  double thps[NUM_ITER_PER_DP];
  int ret = 0;

  int loop = search_depth;

  while(loop --) {
    size_t buffer_size = ((size_t)pivot)<<10;
    ret = sequential_throughput(workspace,buffer_size,NUM_ITER_PER_DP,thps,is_write);
    RETURN_ON_ERROR(ret,"sequential_throughput");
    double v = average(NUM_ITER_PER_DP,thps);
    uint32_t new_pivot = pivot;
    if (v < target_thp) { // go smaller
      ub = pivot;
      new_pivot = (ub + lb) / 2;
    } else { // go bigger
      lb = pivot;
      if (ub == 0)
        new_pivot = pivot << 1;
      else
        new_pivot = (ub + lb) / 2;
    }
    if (pivot == new_pivot) break;
    else pivot = new_pivot;
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
  void *ws;

  boost_cpu();
  ret = posix_memalign(&ws, BUFFER_ALIGNMENT, MEM_ALLOCATION);
  RETURN_ON_ERROR(ret,"posix_memalign()");

  for(i=0;i<num_samples;i++) {
    double thp = lower_thp_GBps + (upper_thp_GBps - lower_thp_GBps) * (i + 1) / (num_samples + 1);
    ret = binary_search (cache_size_hint_KB, thp, is_write, search_depth, ws, &css[i]);
    RETURN_ON_ERROR(ret,"binary_search");
  }

  free (ws);

  return ret;
}
