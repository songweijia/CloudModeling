#!/bin/bash

for f in `ls *.plot`
do
  gnuplot $f
done

for f in `ls *.eps`
do
  fn="${f%.*}"
  epspdf $f
  inkscape $f -l $fn.svg
done
