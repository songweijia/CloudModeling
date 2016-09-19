#!/bin/bash
if [ -d data ]; then
  echo "Please save and move 'data' folder."
  exit 1
fi

mkdir -p data/rr data/stream
# collect result
while read line; do
  provider=`echo $line | awk '{print $1}'`
  vmtype=`echo $line | awk '{print $2}'`
  id=`echo $line | awk '{print $3}'`
  ip=`echo $line | awk '{print $4}'`
  mkdir -p data/rr/$provider/$vmtype data/stream/$provider/$vmtype
  scp -i ../cred/weijia_nvirginia.pem ubuntu@$ip:rr/data/* data/rr/$provider/$vmtype/
  scp -i ../cred/weijia_nvirginia.pem ubuntu@$ip:stream/data/* data/stream/$provider/$vmtype/
done < vm.lst
