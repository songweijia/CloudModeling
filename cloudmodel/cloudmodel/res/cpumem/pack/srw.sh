#!/bin/bash
TIME=`date +%s`
RR_CNT=1000000
ACC_SIZE_MB=256
LOOP=5

cpu_id=$1
cpu_mask=`echo "obase=16; 2^$1" | bc -l`

### WARM UP ###
for ((j=0;j<=100;j++))
do
  sudo taskset 0x${cpu_mask} nice --20 ./srwstride 16 16384 >> data/warmup.result.${cpu_id}
done

for((j=0;j<$LOOP;j++))
do
  for i in `cat buffer_size.lst`
  do
    num=`expr 1024 \* $ACC_SIZE_MB \/ $i`
    sudo taskset 0x${cpu_mask} nice --20 ./srwstride $i $num >> data/srw.result.${cpu_id}
  done
done
