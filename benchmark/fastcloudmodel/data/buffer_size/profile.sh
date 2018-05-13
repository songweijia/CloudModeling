#!/bin/bash

for bs in `cat bs`
do
  sudo ../../cm -e throughput -s ${bs} -n 32 -S 128 | grep WRITE | awk -v bs=$bs '{print bs","$2","$5}'
done
