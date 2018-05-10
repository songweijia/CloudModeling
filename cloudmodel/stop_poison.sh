#!/bin/bash

for pid in `sudo ps -A | grep cache_poisoner | awk '{print $1}'`
do
 echo "sudo kill -9 $pid"
 sudo kill -9 $pid
done
