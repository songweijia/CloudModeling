#!/bin/bash
TIME=`date +%s`
RR_CNT=1000000
ACC_SIZE_MB=256
LOOP=5

### WARM UP ###
for ((j=0;j<=100;j++))
do
  sudo taskset 0x01 nice --20 ./srwstride 16 16384 >> data/warmup.${TIME}
done

:'
for i in `./printx`
do
  num=`expr 1024 \* $ACC_SIZE_MB \/ $i`
  for((j=0;j<$LOOP;j++))
  do
    sudo taskset 0x01 nice --20 ./srwstride $i $num >> data/srw.${TIME}
  done
done
'

for((j=0;j<$LOOP;j++))
do
  for i in `cat buffer_size.lst`
  do
    num=`expr 1024 \* $ACC_SIZE_MB \/ $i`
    sudo taskset 0x01 nice --20 ./srwstride $i $num >> data/srw.${TIME}
  done
done
