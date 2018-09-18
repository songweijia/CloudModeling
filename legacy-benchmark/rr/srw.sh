#!/bin/bash
TIME=`date +%s`
RR_CNT=1000000
ACC_SIZE_GB=1
LOOP=10

for i in `./printx`
do
  num=`expr 1024 \* 1024 \* $ACC_SIZE_GB \/ $i`
  for((j=0;j<=$LOOP;j++))
  do
    sudo taskset 0x01 nice --20 ./srwstride $i $num >> data/srw.${TIME}
  done
done
