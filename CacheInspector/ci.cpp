#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <iostream>
#include <iomanip>

#include "seq_thp.hpp"
#include "cache_size.hpp"
#include "rand_lat.hpp"
#include "util.hpp"

#define HELP_INFO \
 "CacheInspector benchmark\n" \
 "--(e)xperiment throughput|latency|cachesize\n" \
 "--buffer_(s)ize <size in KB>\n" \
 "--(n)um_of_datapoints <num>\n" \
 "--batch_(S)ize <size in MB>\n" \
 "--(u)pper_thp_GBps <20.00>\n" \
 "--(l)ower_thp_GBps <10.00>\n" \
 "--(c)ache_size_hint_KB <20480>\n" \
 "--search_(d)epth <11>\n" \
 "--(N)um_of_thp_dps_per_binary_search <5>\n" \
 "--(L)oop <1> \n" \
 "--show (p)erf counters\n" \
 "--(h)elp\n"

const struct option opts[] = {
  {"experiment",        required_argument,      0, 'e'},
  {"buffer_size",       required_argument,      0, 's'},
  {"num_of_datapoints", required_argument,      0, 'n'},
  {"help",              no_argument,            0, 'h'},
  {"batch_size",        required_argument,      0, 'S'},
  {"upper_thp_GBps",    required_argument,      0, 'u'},
  {"lower_thp_GBps",    required_argument,      0, 'l'},
  {"cache_size_hint_KB",required_argument,      0, 'c'},
  {"search_depth",      required_argument,      0, 'd'},
  {"Loop",              required_argument,      0, 'L'},
  {"Num_of_thp_dps_per_binary_search", required_argument, 0, 'N'},
  {"perf",              no_argument,            0, 'p'},
  {0,0,0,0}
};

enum CMExp {
  EXP_HELP = 0,
  EXP_THROUGHPUT = 1,
  EXP_LATENCY = 2,
  EXP_CACHESIZE
};

static inline void print_timestamp() {
  struct timespec tv;
  if (clock_gettime(CLOCK_REALTIME,&tv) != 0){
    fprintf(stderr, "clock_gettime() failed.\n");
  } else {
    fprintf(stdout, "%ld.%03ld \n", tv.tv_sec,tv.tv_nsec/1000000);
  }
}

int do_throughput(int buffer_size_kb, int num_iter, int batch_size_mb, bool show_perf_counters) {
  double *res = (double*)malloc(sizeof(double)*num_iter);

#if defined(TIMING_WITH_CPU_CYCLES)
    const char *thp_unit = "byte/cycle(CPU)";
#elif defined(TIMING_WITH_RDTSC)
    const char *thp_unit = "byte/cycle(TSC)";
#elif defined(TIMING_WITH_CLOCK_GETTIME)
    const char *thp_unit = "GBps";
#endif

  // boost_cpu
  boost_cpu();

  std::optional<std::vector<std::map<std::string,long long>>> lpcs = std::vector<std::map<std::string, long long>>();
  // read
  if (sequential_throughput(NULL,((size_t)buffer_size_kb)<<10,
        num_iter,res,
        lpcs, 
        0,((uint64_t)batch_size_mb)<<20)) {
    fprintf(stderr, "experiment failed...\n");
  }

  printf("READ %.3f %s std %.3f min %.3f max %.3f\n",
    average(num_iter,res), thp_unit, deviation(num_iter,res),
    minimum(num_iter,res), maximum(num_iter,res));

  std::cout << std::left << std::setw(32) << "throughput per iter";
  if (lpcs.has_value() && show_perf_counters) {
      for (auto itr=(*lpcs)[0].begin(); itr!=(*lpcs)[0].end(); itr++) {
	  std::cout << std::left << std::setw(16) << itr->first;
      }
  }
  printf("\n");

  for (int i=0;i<num_iter;i++) {
      printf("[%d]-%.3f %s\t",i,res[i],thp_unit);
      if (lpcs.has_value() && show_perf_counters){
        for (auto itr=(*lpcs)[i].begin(); itr!=(*lpcs)[i].end(); itr++) {
	    std::cout << std::left << std::setw(16) << itr->second;
        }
      }
      printf("\n");
  }

  // write 
  lpcs->clear();
  if (sequential_throughput(NULL,((size_t)buffer_size_kb)<<10,
        num_iter,res,
        lpcs,
        1,((uint64_t)batch_size_mb)<<20)) {
    fprintf(stderr, "experiment failed...\n");
  }

  printf("WRITE %.3f %s std %.3f min %.3f max %.3f\n",
    average(num_iter,res), thp_unit, deviation(num_iter,res),
    minimum(num_iter,res), maximum(num_iter,res));

  std::cout << std::left << std::setw(32) << "thoughput per iter";
  if (lpcs.has_value() && show_perf_counters) {
      for (auto itr=(*lpcs)[0].begin(); itr!=(*lpcs)[0].end(); itr++) {
	  std::cout << std::left << std::setw(16) << itr->first;
      }
  }
  printf("\n");
  for (int i=0;i<num_iter;i++) {
      printf("[%d]-%.3f %s\t",i,res[i],thp_unit);
      if (lpcs.has_value() && show_perf_counters){
        for (auto itr=(*lpcs)[i].begin(); itr!=(*lpcs)[i].end(); itr++) {
	    std::cout << std::left << std::setw(16) << itr->second;
        }
      }
      printf("\n");
  }
}

int do_cachesize(const uint32_t cache_size_hint_KB,
  const double upper_thp_GBps,
  const double lower_thp_GBps,
  const int num_data_points,
  const int batch_size,
  const bool is_write,
  const int32_t search_depth,
  const int32_t num_of_thp_dps_per_binary_search) {

  uint32_t css[num_data_points];
  assert (upper_thp_GBps > 0.0);
  assert (lower_thp_GBps > 0.0);
  assert (num_data_points > 0);

  int ret = eval_cache_size(cache_size_hint_KB,upper_thp_GBps,lower_thp_GBps,css,num_data_points,is_write,search_depth,num_of_thp_dps_per_binary_search,((uint64_t)batch_size) << 20);
  if(ret) {
    fprintf(stderr,"failed...\n");
    return ret;
  }
  for (int i=0;i<num_data_points;i++){
    printf("[%d]\t%dKB\n",i,css[i]);
  }
  return ret;
}

int do_latency(const int buffer_size_kb,
  const int num_datapoints) {
  double *latencies = (double*)malloc(sizeof(double)*num_datapoints);
  random_latency(((int64_t)buffer_size_kb<<10),num_datapoints,latencies);
  for (int i;i<num_datapoints;i++)
    fprintf(stdout, "%d KB -- %.3f ns\n", buffer_size_kb, latencies[i]);
  free((void*)latencies);
}

int main(int argc, char **argv) {
  int c;
  int option_index = 0;
  int buffer_size_kb = 0;
  int num_datapoints = 0;
  int batch_size_mb = (1<<8);
  enum CMExp exp = EXP_HELP;
  double upper_thp_GBps = 0.0f;
  double lower_thp_GBps = 0.0f;
  uint32_t cache_size_hint_KB = 10240;
  int32_t search_depth = 11;
  int32_t num_of_thp_dps_per_binary_search = 5;
  int nloop = 1;
  bool show_perf_counters = false;
  // parse arguments.
  while(1) {
    c = getopt_long(argc, argv, "e:s:S:n:l:u:c:d:N:L:hp", opts, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'e':
      if (strcmp(optarg,"throughput") == 0)
        exp = EXP_THROUGHPUT;
      else if (strcmp(optarg,"latency") == 0)
        exp = EXP_LATENCY;
      else if (strcmp(optarg,"cachesize") == 0)
        exp = EXP_CACHESIZE;
      else
        exp = EXP_HELP;
      break;

    case 's':
      buffer_size_kb = atoi(optarg);
      break;

    case 'L':
      nloop = atoi(optarg);
      break;

    case 'n':
      num_datapoints = atoi(optarg);
      break;

    case 'S':
      batch_size_mb = atoi(optarg);
      break;

    case 'l':
      lower_thp_GBps = atof(optarg);
      break;

    case 'u':
      upper_thp_GBps = atof(optarg);
      break;

    case 'c':
      cache_size_hint_KB = (uint32_t)atoi(optarg);
      break;

    case 'd':
      search_depth = (int32_t)atoi(optarg);
      break;

    case 'N':
      num_of_thp_dps_per_binary_search = (int32_t)atoi(optarg);
      break;

    case 'p':
      show_perf_counters = true;
      break;

    default:
      printf("skip unknown opt code:0%o ??\n",c);
    }
  }

  switch (exp) {
  case EXP_HELP:
    printf("%s",HELP_INFO);
    break;

  case EXP_THROUGHPUT:
    fprintf(stderr,"=== Throughput Test ===\n");
    fprintf(stderr,"nloop:\t%d\n",nloop);
    fprintf(stderr,"buffer_size_kb:\t%d\n",buffer_size_kb);
    fprintf(stderr,"num_datapoints:\t%d\n",num_datapoints);
    fprintf(stderr,"batch_size_mb:\t%d\n",batch_size_mb);
    while(nloop --)
      do_throughput(buffer_size_kb,num_datapoints,batch_size_mb,show_perf_counters);
    break;

  case EXP_CACHESIZE:
    fprintf(stderr,"=== Cache Size Test ===\n");
    fprintf(stderr,"nloop:\t%d\n",nloop);
    fprintf(stderr,"cache_size_hint_KB:\t%d\n",cache_size_hint_KB);
    fprintf(stderr,"upper_thp_GBps:\t%f\n",upper_thp_GBps);
    fprintf(stderr,"lower_thp_GBps:\t%f\n",lower_thp_GBps);
    fprintf(stderr,"num_datapoints:\t%d\n",num_datapoints);
    fprintf(stderr,"batch_size_mb:\t%d\n",batch_size_mb);
    fprintf(stderr,"search_depth:\t%d\n",search_depth);
    fprintf(stderr,"num_of_thp_dps_per_binary_search:\t%d\n",num_of_thp_dps_per_binary_search);
    while(nloop --){
      // print_timestamp();
      do_cachesize(cache_size_hint_KB,upper_thp_GBps,lower_thp_GBps,num_datapoints,batch_size_mb,true,search_depth,num_of_thp_dps_per_binary_search);
    }
    break;

  case EXP_LATENCY:
    fprintf(stderr,"=== Latency Test ===\n");
    fprintf(stderr,"nloop:\t%d\n",nloop);
    fprintf(stderr,"buffer_size_kb:\t%d\n",buffer_size_kb);
    fprintf(stderr,"num_datapoints:\t%d\n",num_datapoints);
    while(nloop --){
      do_latency(buffer_size_kb,num_datapoints);
    }
    break;
  default:
    ;
  }

  return 0;
}
