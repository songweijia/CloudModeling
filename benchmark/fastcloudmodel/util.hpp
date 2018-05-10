#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#define RETURN_ON_ERROR(err,msg) \
while((err)<0) {\
  fprintf(stderr, "%s, ret=%d, errno=%d(%s)\n", msg, err, errno, strerror(errno));\
  return err;\
}

#define TIMESPAN_NS(s,e) \
  (( (e).tv_sec - (s).tv_sec ) * 1000000000 + ( (e).tv_nsec - (s).tv_nsec ))

#define THROUGHPUT_GBPS(b,s,e) \
  ((double)(b) / TIMESPAN_NS(s,e))

inline double average(int n, double * res) {
  double sum=0.0f;
  for(int i=0;i<n;i++)
    sum+=res[i];
  return sum/n;
}

inline double deviation(int n,double * res) {
  double avg = average(n,res);
  double sum = 0.0f;
  for(int i=0;i<n;i++)
    sum+=(res[i]/avg-1)*(res[i]/avg-1);
  return sqrt(sum/(n-1));
}

inline void boost_cpu() {
  volatile double x = 172612534.3743657287f;
  volatile double y = 237846834.2835876175f;
  uint64_t cnt = (1ull<<20);
  while(cnt--) y*=x;
}

#endif//_UTIL_H_
