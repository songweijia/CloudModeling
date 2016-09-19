#!/bin/bash

# This takes a LONG time to finish.
for f in `ls *.plot`
do
  gnuplot $f
done
