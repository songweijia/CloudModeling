#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>

char *buf;

typedef int v4si __attribute__ ((vector_size (16)));

void main(int argc,char **args){
  if(argc < 2){
    printf("usage:%s <buffersize(KB)> <num>\n",args[0]);
    return;
  }
  else{
    struct timeval tv1,tv2;
    register uint64_t bfsz = atoi(args[1])<<10;
    buf = (char *)malloc(bfsz);
    register uint64_t count = atoi(args[2]);
    register uint64_t i, j;
    register v4si v128;
    register uint64_t v64 = 0x5a5a5a5a5a5a5a5aUL;
    register uint32_t v32 = 0x5a5a5a5a;
    register uint16_t v16 = 0x5a5a;
    register uint8_t v8 =   0x5a;
    for(j=0;j<bfsz;j++){
      buf[j] = 0x5a;
    }

    gettimeofday(&tv1,NULL);
    for(i=0;i<count;i++)
#if W==16
    for(j=0;j<bfsz>>4;j++){
      *((v4si*)buf+j) = v128;
    }
#elif W==8
    for(j=0;j<bfsz>>3;j++){
      *((uint64_t *)buf+j) = v64;
    }
#elif W==4
    for(j=0;j<bfsz>>2;j++){
      *((uint32_t *)buf+j) = v32;
    }
#elif W==2
    for(j=0;j<bfsz>>1;j++){
      *((uint16_t *)buf+j) = v16;
    }
#else
      for(j=0;j<bfsz;j++){
        buf[j] = v8;
      }
#endif

    gettimeofday(&tv2,NULL);
    long elapse = (tv2.tv_sec-tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec);
    printf("%"PRIu64" %.3f\n",(bfsz>>10),((double)bfsz*count)/elapse/1000);
  }
}
