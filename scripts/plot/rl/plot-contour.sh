#!/bin/bash

# This takes a LONG time to finish.
#for vmt in desktop compute31 t2.large c3.large c4.large azure.ds1 azure.ds2 ggl.n1s1 ggl.n1s2 fractusvm
for vmt in m3.medium c3.large c4.large compute31 desktop azure.ds1 azure.ds2 ggl.n1s1 ggl.n1s2 fractusvm
do
  ../getrl.py $vmt rl
  mv rlp.0.0.dat rlp.${vmt}.dat
  cat rlp.plot.template \
  | sed "s/VMTYPE/${vmt}/g" \
  | sed 's/MAXX/1200000/g' \
  | sed 's/MAXY/200/g' \
  > rlp.${vmt}.plot
done

for f in `ls rlp.*.plot`
do
  gnuplot $f
done

for f in `ls lat-*.eps`
do
  epspdf $f
done
