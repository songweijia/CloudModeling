#!/bin/bash

# Get the number of cpus
num_cpu=`cat /proc/cpuinfo  | grep processor | wc -l`
declare -a pids

# Run benchmarks for each cpu core
for((i=0;i<${num_cpu};i++))
do
  ./srw.sh $i &
  pid=$!
  pids[$i]=$pid
done

# wait until all end
for((i=0;i<${num_cpu};i++))
do
  wait ${pids[$i]}
done

