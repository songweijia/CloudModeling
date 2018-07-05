#!/bin/bash
OUTPUT=raw.dat
THPDAT=thp.dat

rm -rf ${OUTPUT}

NDP=`cat bs | wc -l`
for bs in `cat bs`
do
  sudo taskset 0x2 ../cm -e throughput -s ${bs} -n 32 -S 128 | grep WRITE | awk -v bs=$bs '{print $9" "bs}' >> ${OUTPUT}
done

cat ${OUTPUT} | awk '{print $1}' >> ${THPDAT}

# cat ${KMDAT} | kmeans/kmeans | grep ^Cluster\ values | awk '{print $3}' | sort -k 1 -n > profile.res
./kde.py ${THPDAT} >> profile.res

rm -rf ${THPDAT}
