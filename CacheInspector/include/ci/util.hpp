#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

typedef enum {
    CLOCK_GETTIME,
    RDTSC,
    PERF_CPU_CYCLE,
    HW_CPU_CYCLE
} timing_mechanism_t;

#define RETURN_ON_ERROR(err,msg) \
while((err)<0) {\
  fprintf(stderr, "%s, ret=%d, errno=%d(%s)\n", msg, err, errno, strerror(errno));\
  return err;\
}

#define TIMESPAN_NS(s,e) \
  (( (e).tv_sec - (s).tv_sec ) * 1000000000 + ( (e).tv_nsec - (s).tv_nsec ))

#define THROUGHPUT_GiBPS(b,s,e) \
  ((double)(b) / TIMESPAN_NS(s,e) * 1e9 / (1l<<30))

#define THROUGHPUT_BYTES_PER_CYCLE(b,s,e) \
  ((double)(b) / (e-s))

inline double average(int n, const double * res) {
  double sum=0.0f;
  for(int i=0;i<n;i++)
    sum+=res[i];
  return sum/n;
}

inline double deviation(int n,const double * res) {
  double avg = average(n,res);
  double sum = 0.0f;
  for(int i=0;i<n;i++)
    sum+=(res[i]/avg-1)*(res[i]/avg-1);
  return sqrt(sum/(n-1));
}

inline double minimum(int n,const double *res) {
  double min = res[0];
  for(int i=1;i<n;i++)
    if (min > res[i])
      min = res[i];
  return min;
}

inline double maximum(int n,const double *res) {
  double max = res[0];
  for(int i=1;i<n;i++)
    if (max < res[i])
      max = res[i];
  return max;
}

inline void boost_cpu() {
  volatile double x = 172612534.3743657287f;
  volatile double y = 237846834.2835876175f;
  uint64_t cnt = (1ull<<20);
  while(cnt--) y*=x;
}

inline uint64_t get_monotonic_usec() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC,&now);
  return now.tv_sec*1000000 + now.tv_nsec/1000;
}

#endif//_UTIL_H_
