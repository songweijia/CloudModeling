#include <stdio.h>

typedef int v4si __attribute__ ((vector_size(16)));

void main(int argc, char ** argv){
  v4si a,b,c;
  c = a + b;
}
