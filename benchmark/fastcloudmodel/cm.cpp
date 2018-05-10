#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "seq_thp.hpp"
#include "cache_size.hpp"
#include "util.hpp"

#define HELP_INFO \
 "cloudmodel benchmark\n" \
 "--(e)xperiment throughput|latency|cachesize\n" \
 "--buffer_(s)ize <size in KB>\n" \
 "--(n)um_of_datapoints <num>\n" \
 "--batch_(S)ize <size in MB>\n" \
 "--(u)pper_thp_GBps <20.00>\n" \
 "--(l)ower_thp_GBps <10.00>\n" \
 "--(c)ache_size_hint_KB <20480>\n" \
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
  {0,0,0,0}
};

enum CMExp {
  EXP_HELP = 0,
  EXP_THROUGHPUT = 1,
  EXP_LATENCY = 2,
  EXP_CACHESIZE
};

int do_throughput(int buffer_size_kb, int num_iter, int batch_size_mb) {
  double *res = (double*)malloc(sizeof(double)*num_iter);

  // boost_cpu
  boost_cpu();

  // read
  if (sequential_throughput(NULL,((size_t)buffer_size_kb)<<10,
        num_iter,res,0,((uint64_t)batch_size_mb)<<20)) {
    fprintf(stderr, "experiment failed...\n");
  }
  printf("READ %.3f GByte/sec std %.3f\n", average(num_iter,res), deviation(num_iter,res));

  // write 
  if (sequential_throughput(NULL,((size_t)buffer_size_kb)<<10,
        num_iter,res,1,((uint64_t)batch_size_mb)<<20)) {
    fprintf(stderr, "experiment failed...\n");
  }
  printf("WRITE %.3f GByte/sec std %.3f\n", average(num_iter,res), deviation(num_iter,res));
}

int do_cachesize(const uint32_t cache_size_hint_KB,
  const double upper_thp_GBps,
  const double lower_thp_GBps,
  const int num_data_points) {

  uint32_t css[num_data_points];
  assert (upper_thp_GBps > 0.0);
  assert (lower_thp_GBps > 0.0);
  assert (num_data_points > 0);

  int ret = eval_cache_size(cache_size_hint_KB,upper_thp_GBps,lower_thp_GBps,css,num_data_points);
  if(ret) {
    fprintf(stderr,"failed...\n");
    return ret;
  }
  for (int i=0;i<num_data_points;i++){
    printf("[%d]\t%dKB\n",i,css[i]);
  }
  return ret;
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
  // parse arguments.
  while(1) {
    c = getopt_long(argc, argv, "e:s:S:n:l:u:c:h", opts, &option_index);
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

    default:
      printf("skip unknown opt code:0%o ??\n",c);
    }
  }

  switch (exp) {
  case EXP_HELP:
    printf("%s",HELP_INFO);
    break;

  case EXP_THROUGHPUT:
    do_throughput(buffer_size_kb,num_datapoints,batch_size_mb);
    break;

  case EXP_CACHESIZE:
    do_cachesize(cache_size_hint_KB,upper_thp_GBps,lower_thp_GBps,num_datapoints);
    break;

  case EXP_LATENCY:
  default:
  // TODO:
    ;
  }

  return 0;
}
