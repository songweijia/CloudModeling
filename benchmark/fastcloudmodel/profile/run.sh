#!/bin/bash
rm -rf result
mkdir result

for((i=0;i<5;i++))
do
  ./profile.sh 15
  mv profile.time result/profile.time.$i
  mv profile.thp result/profile.thp.$i
  mv profile.lat result/profile.lat.$i
  mv profile.cs result/profile.cs.$i
done
