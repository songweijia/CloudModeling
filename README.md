# **_Overview_**

The **_CloudModeling_** toolkit monitors the unreported/fluctuating resource allocations in the IaaS Cloud services. Currently, we have the designed and implemented **_CacheInspector_**, a lightweight runtime that determines the performance and allocated capacity of shared caches on multi-tenant public clouds. **_CacheInspector_** is easy to support 

# **_CacheInspector_**
**_CacheInspector_** includes a _profiling_ and a _sampling_ phase. In the _profiling_ phase, **_CacheInspector_** determines the throughput and latency of each level of the memory hierarchy. In the _sampling_ phase, **_CacheInspector_** detects, for each cache level,
the cache size available to the application at each point in time. The profiling phase involves introducing increasing pressure in each cache level through a set of carefully crafted microbenchmarks. This process can be time-consuming, but only needs to happen once to determine the performance limits of each cache level and the memory. In contrast, the sampling phase only takes a few 100s milliseconds to run, and repeats periodically to capture changes in allocated cache capacity.

## Installation
1. Install the prerequisite packages:
```
apt install make g++ python-numpy python-scipy
```
2. Setup huge pages:
```
# cat /proc/meminfo | grep -i ^HugePages
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB

# sudo sysctl -w vm.nr_hugepages=512
vm.nr_hugepages = 512

# cat /proc/meminfo | grep -i ^HugePages
HugePages_Total:     512
HugePages_Free:      512
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB

# sudo mkdir /mnt/huge
# sudo mount -t hugetlbfs -o pagesize=2048k,size=2G none /mnt/huge
```

3. Clone and build CloudModeling toolkit source code:
```
# git clone https://github.com/songweijia/CloudModeling
# cd CloudModeling/CacheInspector
# make
```
On successful building, the CacheInspector binary `ci` is ready.

## Profiling
```
# cd profile
# profile.sh
```
You can specify the CPU core (start from 0) for profiling:
```
# profile.sh <core-id>
```
The profiling stage will take tens of minutes. Then you can see the output in the following files:
- *profile.thp* contains the sequential write throughput in GB/s of each level of the memory hierarchy, from the fastest to the slowest.
- *profile.lat* contains the read latency in nanoseconds of each level of the memory hierarchy, from the fastest to the slowest.
- *profile.cs* contains the profiled size of each cache level, from the fastest to the slowest. Notice that the size measurment may change due to the fluctuations of cache allocation.

## Sampling
To sampling the size of a specific cache layer, we need the data in *profile.thp*. 
```
# cat profile.thp
23.180
20.600
17.370
10.130
```
We use the throughput of L3 cache and memory to detect the allocation of L3 cache:
```
# sudo taskset -c 1 ./ci -e cachesize -n 1 -u 17.370 -l 10.130
=== Cache Size Test ===
nloop:	1
cache_size_hint_KB:	10240
upper_thp_GBps:	17.370000
lower_thp_GBps:	10.130000
num_datapoints:	1
batch_size_mb:	256
search_depth:	11
num_of_thp_dps_per_binary_search:	5
[0]	20310KB
```
