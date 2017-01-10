#include <stdio.h>
#include <math.h>

void main(int argc, char ** argv){
  float v = 8.0;
  int i;
  //96 - 64M
  //123 - 1G
  for(i=0;i<94;i++){
    v*=1.1f;
    printf("%.0f\n",roundf(v));
  }
}
