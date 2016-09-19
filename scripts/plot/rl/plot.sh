#!/bin/bash

# This takes a LONG time to finish.
for vmt in desktop compute31 t2.large c3.large c4.large azure.ds1 azure.ds2 ggl.n1s1 ggl.n1s2
do
#  ../getrl.py $vmt rl
#  mv rl.0.0.dat rl.${vmt}.dat
  cat rl.plot.template \
  | sed "s/VMTYPE/${vmt}/g" \
  | sed 's/MAXX/1200000/g' \
  | sed 's/MAXY/150/g' \
  > rl.${vmt}.plot
done

for f in `ls rl.*.plot`
do
  gnuplot $f
done

for f in `ls lat-*.eps`
do
  epspdf $f
done
