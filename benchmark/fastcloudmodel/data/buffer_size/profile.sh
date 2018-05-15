#!/bin/bash
KMDAT=kmeans.dat

cd kmeans
g++ -o kmeans kmeans.cpp
cd ..

NDP=`cat bs | wc -l`
echo $NDP 1 4 100 1 > ${KMDAT}
for bs in `cat bs`
do
  sudo taskset 0x1 ../../cm -e throughput -s ${bs} -n 32 -S 128 | grep WRITE | awk -v bs=$bs '{print $9" "bs}' >> ${KMDAT}
done

cat ${KMDAT} | kmeans/kmeans | grep ^Cluster\ values | awk '{print $3}' | sort -k 1 -n > profile.res
