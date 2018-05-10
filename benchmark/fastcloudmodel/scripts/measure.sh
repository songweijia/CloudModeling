#!/bin/bash
date
sudo taskset 0x2 /home/weijia/workspace/CloudModeling/benchmark/fast_cache_finder/cm -e cachesize -u 17.3 -l 9.96 -S 32 -n 9 -c 20480
