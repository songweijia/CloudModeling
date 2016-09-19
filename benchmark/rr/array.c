#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

//fitting L1 Cache - 8K
//#define SIZE (32)
//fitting L2 Cache - 2M
#define SIZE (512)
//fitting memory - 32M
//#define SIZE (2048)

uint64_t arr[SIZE][SIZE];
#pragma GCC push_options
#pragma GCC optimize ("O0")
uint64_t writeByRow(uint64_t parr[][SIZE], int size, int times){
  register int r,c;
  struct timespec tv1,tv2;
  clock_gettime(CLOCK_REALTIME,&tv1);
  while(times-- > 0){
    for(r = 0; r < size ; r++)
    for(c = 0; c < size ; c++)
      parr[r][c] = 0xabcdabcdabcdabcdL;
  }
  clock_gettime(CLOCK_REALTIME,&tv2);
  return (tv2.tv_sec - tv1.tv_sec)*1000000000L+(tv2.tv_nsec-tv1.tv_nsec);
}

uint64_t writeByCol(uint64_t parr[][SIZE], int size, int times){
  register int r,c;
  struct timespec tv1,tv2;
  clock_gettime(CLOCK_REALTIME,&tv1);
  while(times-- > 0){
    for(c = 0; c < size ; c++)
    for(r = 0; r < size ; r++)
      parr[r][c] = 0xabcdabcdabcdabcdL;
  }
  clock_gettime(CLOCK_REALTIME,&tv2);
  return (tv2.tv_sec - tv1.tv_sec)*1000000000L+(tv2.tv_nsec-tv1.tv_nsec);
}

#pragma GCC pop_options

void main(int argc, char ** argv){
  struct timespec tv1,tv2;
  int times;
  if(argc != 2){
    printf("USAGE:%s <times>\n",argv[0]);
    return;
  }
  //warm up
  writeByRow(arr,SIZE,10);

  times = atoi(argv[1]);
  printf("byrow:%.3fGB/s\n",((double)SIZE*SIZE*8*times/writeByRow(arr,SIZE,times)));
  printf("bycol:%.3fGB/s\n",((double)SIZE*SIZE*8*times/writeByCol(arr,SIZE,times)));
}
