#!/bin/bash

for PSIZE in 128 256 512 1024 2048 3072 4096 5120 6044 7068 8192 12288 16384
do
  ./start_poison.sh ${PSIZE}
  ./get_cs.py
  mv work/data/srw.*.cs work/cs.$PSIZE
  rm -rf work/data
  ./stop_poison.sh
done
