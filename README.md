# **_Overview_**

The **_CloudModeling_** toolkit monitors the unreported/fluctuating resource allocations in the IaaS Cloud services. Currently, we have the designed and implemented **_CacheInspector_**, a lightweight runtime that determines the performance and allocated capacity of shared caches on multi-tenant public clouds.

# **_CacheInspector_**
**_CacheInspector_** includes a _profiling_ and a _sampling_ phase. In the _profiling_ phase, **_CacheInspector_** determines the throughput and latency of each level of the memory hierarchy. In the _sampling_ phase, **_CacheInspector_** detects, for each cache level,
the cache size available to the application at each point in time. The profiling phase involves introducing increasing pressure in each cache level through a set of carefully crafted microbenchmarks. This process can be time-consuming, but only needs to happen once to determine the performance limits of each cache level and the memory. In contrast, the sampling phase only takes a few 100s milliseconds to run, and repeats periodically to capture changes in allocated cache capacity.

## Installation
1. Install the prerequisite python packages:
```
pip3 install numpy scipy matplotlib
```
2. Setup huge page:
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
```

3. Clone and build CloudModeling toolkit source code:
```
# git clone https://github.com/songweijia/CloudModeling
# cd CloudModeling/CacheInspector
# mkdir build
# cmake ..
# make -j
```
On successful building, the CacheInspector binary `ci` is ready. Try `ci --help` to show how to see how to use it.
```
# ci --help
=== CacheInspector Usage ===

1. Cache/Memory throughput test: --sequential_throughput
Compulsory arguments:
   --buffer_size <buffer size in KiB>
Optional arguments:
   [--total_size <total data size in MiB, default is 128MiB>]
   [--num_datapoints <number of data points,default is 32>]
   [--show_perf_counters]
   [--timing_by <clock_gettime|rdtsc|perf_cpu_cycle|hw_cpu_cycle, default is clock_gettime>]

2. Cache/Memory read latency test: --read_latency
Compulsory arguments:
   --buffer_size <buffer size in KiB>
Optional arguments:
   [--num_datapoints <number of data points,default is 32>]
   [--timing_by <clock_gettime|rdtsc|perf_cpu_cycle|hw_cpu_cycle, default is clock_gettime>]
   [--show_perf_counters]

3. Cache/Memory throughput/latency test with schedule: --schedule
Compulsory arguments:
   --schedule_file <schedule file, please see default sample.schedule>
Optional arguments:
   [--timing_by <clock_gettime|rdtsc|perf_cpu_cycle|hw_cpu_cycle, default is clock_gettime>]
   [--show_perf_counters]

4. Cache size test: --cache_size
Compulsory arguments:
   --faster_throughput <the throughput of the faster cache tier in GiB/s (CLOCK_GETTIME) or Bytes/tick (RDTSC) or Bytes/cycle (*_CPU_CYCLE)>
   --slower_throughput <the throughput of the slower cache tier in GiB/s (CLOCK_GETTIME) or Bytes/tick (RDTSC) or Bytes/cycle (*_CPU_CYCLE)>
Optional arguments:
   [--is_write] This option specifies that given throughput numbers are for write. If not specified, those numbers are for read.
   [--cache_size_hint <the hint of the the cache size in KiB, default is 20480>]
   [--num_datapoints <number of data points, default is 1>]
   [--timing_by <clock_gettime|rdtsc|perf_cpu_cycle|hw_cpu_cycle, default is clock_gettime>]

*. Print this message: --help
```
To install cache inspector,
```
sudo make install
```
The binary `ci`, library `libci.a`, headers in `include/ci/`, and profiler script `profile.py` will be installed in `/usr/local`

## Profiling
The profiling stage is defined by a schedule file, which defines the buffer sizes and corresponding throughput and latency test parameters. A schedule file looks as follows:
```
# sample schedule file format:
#(1) buffer_size(bytes)
#(2) enable_thp
#(3) thp_total_data_size(bytes)
#(4) thp_num_datapoints
#(5) enable_lat
#(6) lat_num_datapoints
4096,1,0x8000000,32,1,32
8192,1,0x8000000,32,1,32
9216,1,0x8000000,32,1,32
10240,1,0x8000000,32,1,32
11264,1,0x8000000,32,1,32
12288,1,0x8000000,32,1,32
13312,1,0x8000000,32,1,32
```
Each line of the schedule file, except the commented lines, defines a mini test by six comma-separated attributes: the buffer size (`buffer_size`), throughput test enabler (`enable_thp`), the total data size to test in a throughput test (`thp_total_data_size`), the number of data points of throughput test to collect (`thp_num_datapoints`), latency test enabler(`enable_lat`), the number of data points of latency test to collect(`lat_num_datapoints`). The profiling stage is started by calling `ci` command with the schedule as follows (We need root priviledge to disable processor scheduler. And we suggest pin the process on a fixed core using `taskset`): 
```
# sudo taskset 0x4 ./ci --schedule --schedule_file <schedule> > schedule.output
```
Once it is finished, you can call the profiling tool to get the throughput and latency information of this platform.
```
# profile.py schedule.output
```
It will generate pdf files like the followings:
<table>
  <tr><td colspan="3">Intel Xeon E5-2690 v0</td></tr>
  <tr>
    <td>latency profile</td>
    <td>read throughput profile</td>
  </tr>
  <tr>
    <td><img src=showcase/read_throughput.png></td>
    <td><img src=showcase/latency.png></td>
  </tr>
</table>
<table>
  <tr><td colspan="2">X Gene APM883208 X-1 </td></tr>
  <tr>
    <td>latency profile</td>
    <td>read throughput profile</td>
  </tr>
  <tr>
    <td><img src=showcase/APM883208X1/read_throughput.png></td>
    <td><img src=showcase/APM883208X1/latency.png></td>
  </tr>
</table>

## Sampling
To sampling the size of a specific cache layer, we need the data in the above profiles. 
We use the throughput of L3 cache and memory to detect the allocation of L3 cache:
```
# sudo taskset 0x4 --cache_size --cache_size --faster_throughput 33.53 --slower_throughput 12.95 --num_datapoints 3
search log:
        (20480,26.43), (40960,12.67), (30720,13.11), (25600,14.05), (23040,14.93), (21760,16.55), (21120,19.31), (20800,22.39), (20640,24.27), (20720,23.34), (20760,22.74), (20740,22.87), (20730,0.00), 
[0] 20730 KiB
```
To use the cache inspector in your application, you can include `ci/ci.hpp` in your C/C++ source code and use the following API to decide the size of your cache resource:
```
/**
 * evaluate the cache size for a given instance
 * @param cache_size_hint_KiB
 * @param upper_thp - if we are measuring L2 cache, this is going to be 
 *        the pre-evaluated throughput for L2 cache. The unit of the throughput
 *        is decided by timing configuration used (see config.mk):
 *        1) For 'cpu_cycles', the throughput is in bytes per cpu cycle
 *        2) For 'rdtsc', the throughput is in bytes per tsc cycle
 *        3) For 'clock_gettime', the throughput is in GiB per second
 * @param lower_thp - if we are measuring L3 cache, this is going to be
 *        the pre-evaluated throughput of L3 cache (or memory if no L3 cache)
 * @param css - the output parameter, pointing to an array of uint32_t with
 *        'num_samples' entries. They are in KiB.
 * @param num_samples - the number of evaluated cache sizes. Multiple cache size
 *        estimations give the distribution of the cache size. Let's say the 
 *        throughput of L2 and L3 cache are 20/30GiB/s respectively. the 10GiB/s
 *        gap will be interpolated by 'num_samples' data points. The i-th point
 *        shows where estimation of cache size will be to get this throughput:
 *        20 + 10*i/(num_samples+1) GiB/s throughput. Defaulted to 1.
 * @param is_write - true, if upper_thp/lower_thp is write throughput
 *        False, otherwise.
 * @param timing - timing mechanism: CLOCK_GETTIME | RDTSC | PERF_CPU_CYCLE | HW_CPU_CYCLE
 * @param search_depth - how many iterations through the binary search.
 * @param num_iter_per_sample - how many iteration for each data points we will
 *        run.
 * @param num_bytes_per_iter - how many bytes to scan, this will be passed to
 *        sequential_throughput() in bytes_per_iter. Default to 256MiB.
 * @param buf - the caller can provide a buffer for test. This parameter is useful when running cache_size detector
 *        periodically to avoid reallocating memory buffer in size eval_cache_size(), which takes hundreds of
 *        milliseconds to warm up.
 * @param buf_size - the size of the 'buf' provided by caller. We suggest size 256 MiB.
 * @param warm_up_cpu - do we need to warm up the cpu before testing? In systems, especailly those CPU is managed by a
 *        'powersave' policy, the CPU cores are generally running at a frequency as low as possible. This will affect
 *        the performance results. Assuming we use CLOCK_GETTIME timing mechanism, if the CPU is running at 2.9GHz, the
 *        L1 throughput will only be 3/4 of that when CPU is running at 3.8GHz. What about using HW_CPU_CYCLES? Changing
 *        CPU frequency is problematic still because memory response time is not affected by CPU clock, and the
 *        evaluated speed for memory throughput is 31 percent faster with 2.9GHz than with 3.8GHz. Since our cache size
 *        evaluation mechanism relies heavily on stable CPU frequency, we suggest always boost the cpu to its highest
 *        frequecy before testing. This can be avoid if the caller is sure about a stable CPU frequency.
 * @return 0 for success, otherwise failure
 */
extern int eval_cache_size(
        const uint32_t cache_size_hint_KiB,
        const double upper_thp,
        const double lower_thp,
        uint32_t* css,
        const int num_samples = 1,
        const bool is_write = true,
        const timing_mechanism_t timing = CLOCK_GETTIME,
        const int32_t search_depth = 12,
        const uint32_t num_iter_per_sample = 5,
        const uint64_t num_bytes_per_iter = (1ull << 28),
        void* buf = nullptr,
        const uint64_t buf_size = 0,
        const bool warm_up_cpu = true);
```
Please use `src/ci.cpp` as an example. The document is the comments inside source and header files.
