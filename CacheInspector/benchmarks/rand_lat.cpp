#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <assert.h>
#include <sched.h>
#include <stdlib.h>
#include "rand_lat.hpp"

static bool initialize_random_seed() {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
    fprintf(stderr,"clock_gettime() failed.\n");
    return false;
  }
  srand((unsigned int)ts.tv_nsec);
  return true;
}

static bool fill_cyclic_linked_list(uint64_t *cll, int64_t ecnt) {
  uint8_t *bytemap = (uint8_t*)malloc(ecnt);
  if (!bytemap) {
    fprintf(stderr,"failed to get byte map.\n");
    return false;
  }

  bzero((void*)bytemap,ecnt);
  if (!initialize_random_seed()) {
    return false;
  }

  int64_t head = rand()%ecnt;
  int64_t offset = head;
  bytemap[offset] = 1;
  int64_t filled = 1;
  while(filled < ecnt) {
    int64_t next = random()%ecnt;
    while(bytemap[next]) next = (next+1)%ecnt;
    cll[offset] = (uint64_t)&cll[next];
    offset = next;
    bytemap[offset] = 1;
    filled ++;
  }
  cll[offset] = (uint64_t)&cll[head];

  free(bytemap);
  return true;
}

static float traverse_cyclic_linked_list (int64_t num, uint64_t *cll ) {
  struct timespec t1,t2;
  if(clock_gettime(CLOCK_MONOTONIC,&t1) < 0) {
    fprintf(stderr, "failed to call clock_gettime().\n");
    return 0.0f;
  }
  // STEP 1 - traverse
  __asm__ volatile (
    "movq	%0, %%rax \n\t"
    "movq	%1, %%rbx \n\t"
"begin_traverse%=: \n\t"
    //load data
    "subq	$128, %%rax \n\t"
    "js		done_traverse%= \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "movq	(%%rbx), %%rbx \n\t"
    "jmp	begin_traverse%=\n\t"
"done_traverse%=: \n\t"
    : // no output
    : "m"(num), "m"(cll)
    : "rax","rbx"
  );
  if(clock_gettime(CLOCK_MONOTONIC,&t2) < 0) {
    fprintf(stderr, "failed to call clock_gettime().\n");
    return 0.0f;
  }
  return (double)(t2.tv_sec-t1.tv_sec)*1e9 + (double)(t2.tv_nsec - t1.tv_nsec);
}

void random_latency(int64_t buffer_size,int num_points,double *output) {
  // STEP 1 - prepare the cyclic linked list
  uint64_t * cll;
  int64_t num_entries = buffer_size/sizeof(uint64_t);
  if (posix_memalign((void**)&cll, 4096, buffer_size)!=0) {
    fprintf(stderr, "fail to call posix_memalign. errno=%d\n", errno);
  }
  if (fill_cyclic_linked_list(cll,num_entries)==false) {
    fprintf(stderr, "failed to fill the cyclic linked list.\n");
  }

  // STEP 2 - disable scheduler
  int max_pri = sched_get_priority_max(SCHED_FIFO);
  if(max_pri < 0) {
    fprintf(stderr, "sched_get_priority_max() failed. errno=%d\n",errno);
  }
  struct sched_param sch_parm;
  sch_parm.sched_priority = max_pri;
  if(sched_setscheduler(0,SCHED_FIFO,&sch_parm) != 0) {
    fprintf(stderr, "sched_setscheduler() failed.\n");
  }

  // STEP 3 - warm_up by scan the buffer 5 times.
  for(int l=0;l<5;l++) {
    for(int i=0;i<num_entries;i+=8) {
      volatile register uint64_t rx;
      rx = cll[i];
    }
  }

  // STEP 4 - test
  #define NUMBER_OF_ACCESS (16<<10)
  for(int i=0;i<num_points;i++)
    output[i] = traverse_cyclic_linked_list(NUMBER_OF_ACCESS,cll+(i%num_entries))/NUMBER_OF_ACCESS;

  // STEP 5 - clean up
  free(cll);
}

#ifdef UNIT_TEST
int main(int argc, char **argv) {
  if (argc != 2){
    printf("Usage: %s <buffer_size>.\n",argv[0]);
    return 0;
  }

  int buffer_size = atoi(argv[1]);
  //int num_entries = atoi(argv[1]);

  //uint64_t* entries = (uint64_t *)malloc(num_entries*sizeof(uint64_t));
  //if (fill_cyclic_linked_list(entries,num_entries)==false) {
  //  fprintf(stderr,"failed to allocate cyclic linked list.\n");
  //  return -1;
  //}

  //for (int i=0;i<num_entries;i++) {
  //  fprintf(stderr,"%d --> %ld\n",i,(uint64_t*)entries[i]-entries);
  //}

  //free(entries);

  printf("latency = %.3f\n",random_latency(buffer_size));

  return 0;
}
#endif//UNIT_TEST
