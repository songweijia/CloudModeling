#!/bin/bash

for vmt in "desktop" "compute31" "t2.large" "c3.large" "c4.large" "azure.ds1" "ggl.n1s1" "ggl.n1s2" "azure.ds2"
do
  for bfsz in `cat rlr_bfsz.lst`
  do
    cat rl.${vmt}.dat | grep "^$bfsz " | awk '{print $2" "$3" "$4}' > rlr.${vmt}.${bfsz}.dat
  done
done
