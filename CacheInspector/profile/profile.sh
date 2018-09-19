#!/bin/bash
if [ $# -gt 0 ]; then
  CORE_ID=$1
else
  CORE_ID=0
fi


START_TIME=`date +%s`

OUTPUT=raw.dat
THPDAT=thp.dat

CSH=(`lscpu | grep cache | sed '/L1i/d' | awk '{print $3}' | sed 's/K//'`)

# step 1 throughput
rm -rf ${OUTPUT}

NDP=`cat bs | wc -l`
for bs in `cat bs`
do
  sudo taskset -c ${CORE_ID} ../ci -e throughput -s ${bs} -n 32 -S 128 | grep WRITE | awk -v bs=$bs '{print $9" "bs}' >> ${OUTPUT}
done

cat ${OUTPUT} | awk '{print $1}' >> ${THPDAT}

# cat ${KMDAT} | kmeans/kmeans | grep ^Cluster\ values | awk '{print $3}' | sort -k 1 -n > profile.res
./kde.py ${THPDAT} ${#CSH[@]} > profile.thp

THP_TIME=`date +%s`
rm -rf ${THPDAT}

# step 2 find cache size
CS=(`cat profile.thp`)

for((i=0;i<${#CSH[@]};i++))
do
  let l=$i+1
  buffer_size=`sudo taskset -c ${CORE_ID} ../ci -e cachesize -S 128 -u ${CS[$i]} -l ${CS[$l]} -c ${CSH[$i]} -n 1 -N 10 | head -1 | awk '{print $2}' | sed 's/KB//'`
  test_buffer_size=`expr ${buffer_size} \* 4 \/ 5`
  echo ${buffer_size} >> profile.cs
  # test_buffer_size=$buffer_size
  # sudo taskset -c ${CORE_ID} ./rr_lat ${test_buffer_size} 20 | awk '{SUM+=$3} END {print SUM / NR " ns"}' >> profile.lat
  sudo taskset -c ${CORE_ID} ../ci -e latency -s ${test_buffer_size} -n 200 | awk '{SUM+=$4}END{print SUM/NR " ns"}' >> profile.lat
done

buffer_size=`sudo taskset -c ${CORE_ID} ../ci -e cachesize -S 128 -u ${CS[-2]} -l ${CS[-1]} -c ${CSH[-1]} -n 1 -N 10 | tail -1 | awk '{print $2}' | sed 's/KB//'`
test_buffer_size=`expr ${buffer_size} \* 5`
# sudo taskset -c ${CORE_ID} ./rr_lat ${test_buffer_size} 20 | awk '{SUM+=$3} END {print SUM / NR " ns"}' >> profile.lat
sudo taskset -c ${CORE_ID} ../ci -e latency -s ${test_buffer_size} -n 5 | awk '{SUM+=$4}END{print SUM/NR " ns"}' >> profile.lat

LAT_TIME=`date +%s`

expr $THP_TIME - $START_TIME >> profile.time
expr $LAT_TIME - $THP_TIME >> profile.time
