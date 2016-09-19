#!/bin/bash

./get2ddata.sh

for vmt in desktop compute31 t2.large c3.large c4.large azure.ds1 ggl.n1s1 azure.ds2 ggl.n1s2
do
  cat rlr.plot.template \
  | sed "s/VMTYPE/${vmt}/g" \
  | sed 's/MAXX/160/g' \
  > rlr.${vmt}.plot
done

for f in `ls rlr.*.plot`
do
  gnuplot $f
done

for f in `ls *-2d.eps`
do
  epspdf $f
done
