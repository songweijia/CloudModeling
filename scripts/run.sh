#!/bin/bash

VMSFILE=vm.lst

### create vms
#for vmtype in t2.nano t2.micro t2.small t2.medium t2.large
#for vmtype in t2.nano t2.micro t2.small
for vmtype in t2.medium t2.large c3.large c3.2xlarge
do
  ./newinstance.sh amz $vmtype >> vm.lst
done
sleep 60

### deploy benchmarks
provider_arr=()
vmtype_arr=()
id_arr=()
ip_arr=()
let idx=0
while read line; do
  provider=`echo $line | awk '{print $1}'`
  vmtype=`echo $line | awk '{print $2}'`
  id=`echo $line | awk '{print $3}'`
  ip=`echo $line | awk '{print $4}'`
  provider_arr[$idx]=$provider
  vmtype_arr[$idx]=$vmtype
  id_arr[$idx]=$id
  ip_arr[$idx]=$ip
  let idx=$idx+1
done < vm.lst

for((i=0;i<${idx};i++))
do
  echo "type:${vmtype_arr[$i]} id:${id_arr[$i]} start."
  ./deploy.sh ${ip_arr[$i]} ubuntu weijia_nvirginia.pem ${provider_arr[$i]} ${vmtype_arr[$i]}
  echo "type:${vmtype_arr[$i]} id:${id_arr[$i]} done."
done
