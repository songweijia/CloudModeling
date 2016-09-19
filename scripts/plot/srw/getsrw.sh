#!/bin/bash
# DESC:  get sequential log file
# USAGE: getsrw.sh <file>

if [[ $# != 1 ]]; then
  echo "USAGE: $0 <srw log file>"
  exit -1
fi

srw_file=$1
raw_file=srw_raw.dat
avg_file=srw_avg.dat
max_file=srw_max.dat

# STEP 1 get data
cat ${srw_file} | awk '{print $2" "$5" "$8" "$11" "$14}' > ${raw_file}

# STEP 2 get average
for bs in `cat ${raw_file} | awk '{print $1}' | uniq`
do
  cat ${raw_file} | grep "^${bs} " | awk -v BS="${bs}" '{RSUM+=$2;WSUM+=$3;SRS+=$4;SWS+=$5}END{print BS" "RSUM/NR" "WSUM/NR" "SRS/NR" "SWS/NR}' >> ${avg_file}
  cat ${raw_file} | grep "^${bs} " | awk -v BS="${bs}" -v RMAX=0 -v WMAX=0 -v SRMAX=0 -v SWMAX=0 '{if($2>RMAX)RMAX=$2;if($3>WMAX)WMAX=$3;if($3>SRMAX)SRMAX=$3;if($4>SWMAX)SWMAX=$4;}END{print BS" "RMAX" "WMAX" "SRMAX" "SWMAX}' >> ${max_file}
#  cat ${raw_file} | grep "^${bs} " | awk -v BS="${bs}" -v RMAX=0 -v WMAX=0 -v SRMAX=0 -v SWMAX=0 '{if($2>RMAX)RMAX=$2;if($3>WMAX)WMAX=$3;if($3>SRMAX)SRMAX=$3;if($4>SWMAX)SWMAX=$4}END{print BS}' >> ${max_file}
done
