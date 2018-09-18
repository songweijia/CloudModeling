#!/bin/bash
TIME=`date +%s`
RR_CNT=1000000
ACC_SIZE_GB=2

for i in `./printx`
do
#  ./rand_read $i $RR_CNT >> data/rr.${TIME}
#  ./seq_write $i `expr 1024 \* 1024 \* $ACC_SIZE_GB \/ $i` >> data/sw.${TIME}
  sudo taskset 0x01 nice --20 ./rr_lat $i 100000 >> data/rl.${TIME}
done
