#include <stdlib.h>
#include "cache_size.hpp"
#include "seq_thp.hpp"
#include "util.hpp"

#define BUFFER_ALIGNMENT        (4096)
#define MEM_ALLOCATION		(256ull << 20)

static int32_t binary_search (
#ifdef LOG_BINARY_SEARCH
  int32_t *bs_log,
  const int32_t tot_search_depth,
#endif//LOG_BINARY_SEARCH
  const uint32_t seed_kb,
  const double * target_thps,
  const int num_samples,
  const bool is_write,
  const int32_t search_depth,
  void * workspace, // a memory workspace with MEM_ALLOCATION 256MB.
  uint32_t *output,
  const int32_t num_iter_per_sample,
  const uint64_t num_bytes_per_iter,
  const uint32_t LB = 0ul,
  const uint32_t UB = 0ul) {

  uint32_t lb = LB;
  uint32_t ub = UB;

  // done
  if (num_samples == 0)
    return 0;

  // get the middle target
  const double target_thp = target_thps[num_samples/2];

  // search ...
  uint32_t pivot = seed_kb;
  double thps[num_iter_per_sample];
  uint32_t ret = 0;

  int loop = search_depth;
#ifdef LOG_BINARY_SEARCH
  bs_log[(tot_search_depth-loop) + (tot_search_depth+1)*(num_samples/2)] = pivot;
#endif//LOG_BINARY_SEARCH
  while(loop --) {
    size_t buffer_size = ((size_t)pivot)<<10;
    ret = sequential_throughput(workspace,buffer_size,
      num_iter_per_sample,thps,is_write,num_bytes_per_iter);
    RETURN_ON_ERROR(ret,"sequential_throughput");
    double v = average(num_iter_per_sample,thps);
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
#ifdef LOG_BINARY_SEARCH
  bs_log[(tot_search_depth-loop) + (tot_search_depth+1)*(num_samples/2)] = pivot;
#endif//LOG_BINARY_SEARCH
  }

  output[num_samples/2] = pivot;

  // search others
  uint32_t npivot = (lb + pivot)/2;
  uint32_t nofst = 0;
  uint32_t nlen = num_samples/2;
  uint32_t nlb = LB;
  uint32_t nub = pivot;
  ret = binary_search(
#ifdef LOG_BINARY_SEARCH
    bs_log,
    tot_search_depth,
#endif//LOG_BINARY_SEARCH
    npivot,
    target_thps + nofst,nlen,is_write,
    search_depth-1,workspace,output+nofst,
    num_iter_per_sample,num_bytes_per_iter,nlb,nub);
  RETURN_ON_ERROR(ret,"binary_search, upper half");

  npivot = UB?(pivot+UB)/2:2*pivot;
  nofst = num_samples/2+1;
  nlen = num_samples-num_samples/2-1;
  nlb = pivot;
  nub = ub;
  ret = binary_search(
#ifdef LOG_BINARY_SEARCH
    bs_log,
    tot_search_depth,
#endif//LOG_BINARY_SEARCH
    npivot,
    target_thps + nofst,nlen,is_write,
    search_depth-1,workspace,output+nofst,
    num_iter_per_sample,num_bytes_per_iter,nlb,nub);
  RETURN_ON_ERROR(ret,"binary_search, lower half");
  
  return ret;
}

int eval_cache_size(
  const uint32_t cache_size_hint_KB,
  const double upper_thp_GBps,
  const double lower_thp_GBps,
  uint32_t * css,
  const int num_samples,
  const bool is_write,
  const int32_t search_depth,
  const uint32_t num_iter_per_sample,
  const uint64_t num_bytes_per_iter
  ) {

  int i,ret;
  void *ws;

  boost_cpu();
  ret = posix_memalign(&ws, BUFFER_ALIGNMENT, MEM_ALLOCATION);
  RETURN_ON_ERROR(ret,"posix_memalign()");

  // printf("L3 Write Throughput: %.3f GB/s.\n", upper_thp_GBps);
  // printf("Memory Write Throughput: %.3f GB/s.\n",lower_thp_GBps);
  double thps[num_samples];
  for(i=0;i<num_samples;i++)
    thps[i] = upper_thp_GBps - (upper_thp_GBps - lower_thp_GBps) * (i + 1) / (num_samples + 1);

#ifdef LOG_BINARY_SEARCH
  const int num_log_entry = num_samples*(search_depth+1);
  int32_t *bs_log = (int32_t*)malloc(num_log_entry*sizeof(int32_t));
  if(bs_log == nullptr) {
    fprintf(stderr, "failed to allocate log entry.\n");
    return -1;
  }
  bzero((void*)bs_log,num_log_entry*sizeof(int32_t));
#endif

  ret = binary_search (
#ifdef LOG_BINARY_SEARCH
    bs_log,
    search_depth,
#endif//LOG_BINARY_SEARCH
    cache_size_hint_KB, thps, num_samples, is_write, search_depth, ws, css, num_iter_per_sample, num_bytes_per_iter);
  RETURN_ON_ERROR(ret,"binary_search");

#ifdef LOG_BINARY_SEARCH
  printf("search log:\n");
  for (int i = 0; i < num_samples; i++)
  {
    printf("\t");
    for (int j = 0; j <= search_depth; j++)
      printf("%d,",bs_log[i*(search_depth+1)+j]);
    printf("\n");
  }
#endif//LOG_BINARY_SEARCH

  free (ws);

  return ret;
}
