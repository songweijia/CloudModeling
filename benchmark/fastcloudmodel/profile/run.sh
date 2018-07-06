#!/bin/bash

for((i=0;i<5;i++))
do
  ./profile.sh 15
  mv profile.time profile.time.$i
  mv profile.thp profile.thp.$i
  mv profile.lat profile.lat.$i
  mv profile.cs profile.cs.$i
done
