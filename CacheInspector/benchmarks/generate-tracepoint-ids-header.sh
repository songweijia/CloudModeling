#!/bin/bash
HEADER_FILE=linux_perf_tracepoint_ids.hpp
rm -f $HEADER_FILE
echo "#ifndef LINUX_PERF_TRACEPOINT_IDS_HPP" >> $HEADER_FILE
echo "#define LINUX_PERF_TRACEPOINT_IDS_HPP" >> $HEADER_FILE
echo "" >> $HEADER_FILE
for event in `sudo perf list tracepoint | grep Tracepoint\ event | sed 's/:/\//g' | awk '{print $1}'`
do
    echo -n -e "${event}\t"| sed -E 's/\/|-/_/g' | awk '{printf "#define %s ", toupper($1)}' >> $HEADER_FILE 
    sudo cat `sudo find /sys/kernel/ | grep "${event}/id"` >> $HEADER_FILE
done
echo "" >> $HEADER_FILE
echo "#endif" >> $HEADER_FILE

clang-format -i $HEADER_FILE
