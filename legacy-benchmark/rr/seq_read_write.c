#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <malloc.h>

volatile char *buf;

typedef int v4si __attribute__ ((vector_size (16)));

void init(uint64_t *buf, int blen){
  int cnt = blen >> 3;
  while(cnt-->0)
    buf[cnt]=0xa5a5a5a5a5a5a5a5l;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")
uint64_t doRead(uint64_t *rbuf, int rlen, int times){
  register uint64_t * buf = rbuf;
  register int len = rlen;
  register int cnt;
  register uint64_t r8 __asm("r8");
  register uint64_t r9 __asm("r9");
  register uint64_t r10 __asm("r10");
  register uint64_t r11 __asm("r11");
  register uint64_t r12 __asm("r12");
  register uint64_t r13 __asm("r13");
  register uint64_t r14 __asm("r14");
  register uint64_t r15 __asm("r15");
  struct timespec t1,t2;
  clock_gettime(CLOCK_REALTIME, &t1);
  while(times-->0){
    cnt = 0;
    while(cnt < len){
      r8  = buf[cnt];
      r9  = buf[cnt+1];
      r10 = buf[cnt+2];
      r11 = buf[cnt+3];
      r12 = buf[cnt+4];
      r13 = buf[cnt+5];
      r14 = buf[cnt+6];
      r15 = buf[cnt+7];
      cnt += 8;
    }
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  volatile uint64_t x = r8|r9|r10|r11|r12|r13|r14|r15;
  return (t2.tv_sec-t1.tv_sec)*1000000000L + (t2.tv_nsec - t1.tv_nsec);
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize ("O0")
uint64_t doStrideRead(uint64_t *rbuf, int rlen, int times){
  register uint64_t * buf=rbuf;
  register int len = rlen;
  register int cnt;
  register int ofst;
  register uint64_t r8 __asm("r8");
  register uint64_t r9 __asm("r9");
  register uint64_t r10 __asm("r10");
  register uint64_t r11 __asm("r11");
  register uint64_t r12 __asm("r12");
  register uint64_t r13 __asm("r13");
  register uint64_t r14 __asm("r14");
  register uint64_t r15 __asm("r15");
  struct timespec t1,t2;
  clock_gettime(CLOCK_REALTIME, &t1);
  while(times-->0){
    ofst = 0;
    while(ofst++ < 8){
      cnt = ofst-1;
      while(cnt < len){
        r8  = buf[cnt];
        r9  = buf[cnt+8];
        r10 = buf[cnt+16];
        r11 = buf[cnt+24];
        r12 = buf[cnt+32];
        r13 = buf[cnt+40];
        r14 = buf[cnt+48];
        r15 = buf[cnt+56];
        cnt += 64;
      }
    }
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  volatile uint64_t x = r8|r9|r10|r11|r12|r13|r14|r15;
  return (t2.tv_sec-t1.tv_sec)*1000000000L + (t2.tv_nsec - t1.tv_nsec);
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize ("O0")
uint64_t doWrit(uint64_t *rbuf, int rlen, int times){
  register uint64_t * buf = rbuf;
  register int len = rlen;
  register int cnt;
  register uint64_t r8 __asm("r8");
  register uint64_t r9 __asm("r9");
  register uint64_t r10 __asm("r10");
  register uint64_t r11 __asm("r11");
  register uint64_t r12 __asm("r12");
  register uint64_t r13 __asm("r13");
  register uint64_t r14 __asm("r14");
  register uint64_t r15 __asm("r15");

  r8= 0x5a5a5a5a5a5a5a00l;
  r9= 0x5a5a5a5a5a5a5a01l;
  r10=0x5a5a5a5a5a5a5a02l;
  r11=0x5a5a5a5a5a5a5a03l;
  r12=0x5a5a5a5a5a5a5a04l;
  r13=0x5a5a5a5a5a5a5a05l;
  r14=0x5a5a5a5a5a5a5a06l;
  r15=0x5a5a5a5a5a5a5a07l;
  struct timespec t1,t2;
  clock_gettime(CLOCK_REALTIME, &t1);
  while(times-->0){
    cnt = 0;
    while(cnt < len){
      buf[cnt] = r8;
      buf[cnt+1] = r9;
      buf[cnt+2] = r10;
      buf[cnt+3] = r11;
      buf[cnt+4] = r12;
      buf[cnt+5] = r13;
      buf[cnt+6] = r14;
      buf[cnt+7] = r15;
      cnt += 8;
    }
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  return (t2.tv_sec-t1.tv_sec)*1000000000L + (t2.tv_nsec - t1.tv_nsec);
}
#pragma GCC pop_options

#pragma GCC push_options
#pragma GCC optimize ("O0")
uint64_t doStrideWrit(uint64_t *rbuf, int rlen, int times){
  register uint64_t * buf=rbuf;
  register int len = rlen;
  register int cnt;
  register int ofst;
  register uint64_t r8 __asm("r8");
  register uint64_t r9 __asm("r9");
  register uint64_t r10 __asm("r10");
  register uint64_t r11 __asm("r11");
  register uint64_t r12 __asm("r12");
  register uint64_t r13 __asm("r13");
  register uint64_t r14 __asm("r14");
  register uint64_t r15 __asm("r15");

  r8= 0x5a5a5a5a5a5a5a00l;
  r9= 0x5a5a5a5a5a5a5a01l;
  r10=0x5a5a5a5a5a5a5a02l;
  r11=0x5a5a5a5a5a5a5a03l;
  r12=0x5a5a5a5a5a5a5a04l;
  r13=0x5a5a5a5a5a5a5a05l;
  r14=0x5a5a5a5a5a5a5a06l;
  r15=0x5a5a5a5a5a5a5a07l;
  struct timespec t1,t2;
  clock_gettime(CLOCK_REALTIME, &t1);
  while(times-->0){
    ofst = 0;
    while(ofst++ < 8){
      cnt = ofst-1;
      while(cnt < len){
        buf[cnt] = r8;
        buf[cnt+8] = r9;
        buf[cnt+16] = r10;
        buf[cnt+24] = r11;
        buf[cnt+32] = r12;
        buf[cnt+40] = r13;
        buf[cnt+48] = r14;
        buf[cnt+56] = r15;
        cnt += 64;
      }
    }
  }
  clock_gettime(CLOCK_REALTIME, &t2);
  volatile uint64_t x = r8|r9|r10|r11|r12|r13|r14|r15;
  return (t2.tv_sec-t1.tv_sec)*1000000000L + (t2.tv_nsec - t1.tv_nsec);
}
#pragma GCC pop_options



void main(int argc,char **args){
  if(argc < 3){
    printf("usage:%s <buffersize(KB)> <num>\n",args[0]);
    return;
  }

  int bfsz = atoi(args[1])<<10;
  int num = atoi(args[2]);
  uint64_t * buf = (uint64_t *)memalign(1024,bfsz+1);
  if(buf == NULL){
    fprintf(stderr,"memalign() failed...exit.\n");
    return;
  }

  init(buf,bfsz);

  uint64_t read_ns = doRead(buf,bfsz>>3,num);
  uint64_t writ_ns = doWrit(buf,bfsz>>3,num);
  uint64_t sread_ns = doStrideRead(buf,bfsz>>3,num);
  uint64_t swrit_ns = doStrideWrit(buf,bfsz>>3,num);

  printf("buffer_size: %u KB\tread: %.3f GB/s\twrite: %.3f GB/s stride_read: %.3f GB/s stride_writ: %.3f GB/s\n",
    (bfsz>>10),
    ((double)bfsz*num)/read_ns,
    ((double)bfsz*num)/writ_ns,
    ((double)bfsz*num)/sread_ns,
    ((double)bfsz*num)/swrit_ns
    );
}
