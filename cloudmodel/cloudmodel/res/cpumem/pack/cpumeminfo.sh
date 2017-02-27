#!/bin/bash
# get and memory information and write to a file called cpumem.info
outfile=cpumem.info
let idx=1

# get cpu model info
echo "${idx}. CPU MODEL" >> ${outfile}
cat /proc/cpuinfo | grep "model\ name" | head -1 >> ${outfile}
let idx=${idx}+1
echo "" >> ${outfile}

# get cpu topology
echo "${idx}. CPU TOPOLOGY" >> ${outfile}
ncpu=`cat /proc/cpuinfo | grep "model\ name" | wc -l`
for((i=0;i<${ncpu};i++))
do
  core_id=`cat /sys/devices/system/cpu/cpu${i}/topology/core_id`
  phy_id=`cat /sys/devices/system/cpu/cpu${i}/topology/physical_package_id`
  echo "  cpu${i} ${core_id} ${phy_id}" >> ${outfile}
done
let idx=${idx}+1
echo "" >> ${outfile}

# get cpu cache
echo "${idx}. CPU CACHE" >> ${outfile}
for((i=0;i<${ncpu};i++))
do
  echo "  ====" >> ${outfile}
  echo "  cpu${i}" >> ${outfile}
  echo "  ====" >> ${outfile}
  for cache in `ls /sys/devices/system/cpu/cpu${i}/cache`
  do
    folder="/sys/devices/system/cpu/cpu${i}/cache/${cache}"
    l=`cat ${folder}/level`
    t=`cat ${folder}/type`
    s=`cat ${folder}/size`
    nset=`cat ${folder}/number_of_sets`
    nway=`cat ${folder}/ways_of_associativity`
    lsz=`cat ${folder}/coherency_line_size`
    cpumap=`cat ${folder}/shared_cpu_map`
    echo "  L${l} ${t} ${s} ${nset}-set ${nway}-way ${lsz}B-cacheline ${cpumap}" >> ${outfile}
  done
done
let idx=${idx}+1
echo "" >> ${outfile}

# get mem info
echo "${idx}. MEMORY SIZE" >> ${outfile}
mem_size=`cat /proc/meminfo | grep MemTotal | awk '{print $2}'`
echo "${mem_size}K" >> ${outfile}
