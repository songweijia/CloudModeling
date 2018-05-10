#!/bin/bash
for((i=8;i<22;i++))
do
  taskset -c $i ./cache_poisoner $1 &
done
ps -A | grep poison
