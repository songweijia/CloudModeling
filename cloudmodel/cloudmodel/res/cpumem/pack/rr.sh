#!/bin/bash

L1C=$1
L2C=$2
L3C=$3
MEM=$4

sudo taskset 0x01 nice --20 ./rr_lat ${L1C} 30 > data/rr0.result
sudo taskset 0x01 nice --20 ./rr_lat ${L2C} 20 > data/rr1.result
sudo taskset 0x01 nice --20 ./rr_lat ${L3C} 10 > data/rr2.result
sudo taskset 0x01 nice --20 ./rr_lat ${MEM} 5 > data/rr3.result
