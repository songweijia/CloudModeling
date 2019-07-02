# toggle the options below to enable/disable the features

# With hugepage, the benchmark is less vulnerable to TLB thrashing.
# using hugepage requires huge page being enabled before hand
# 'sudo sysctl -w vm.nr_hugepages=1024'
ENABLE_HUGEPAGE = 0

# choose the timeing services
# 1 - clock_gettime: using Linux clock_gettime() system call. "clock_gettime"
#     is quite heavy compared to the following timing services. But its wallclock
#     readings are convenient. 
# 2 - rdtsc: using rdtsc instruction. Please note that rdtsc may not represent
#     the real CPU Cyles since the new processors has equipped by "constant rate
#     TSC", which is not affected by frequncy scaling.
# 3 - cpu_cycles: using perf's cpu cycle event. The cpu cycle counter is
#     a hardware resource which is not always available to container or virtual
#     machines.
TIMING_SERVICE = cpu_cycles

# show perf cpu cycles
SHOW_PERF_CPU_CYCLES = 1

# show performance counters during the tests.
SHOW_PERF_SCHED_SWITCH = 0

# - hardware counters in intel cpu:
SHOW_INTEL_CPU_CYCLES = 0
SHOW_INTEL_REF_CYCLES = 1
SHOW_INTEL_LLC_HITS = 0
SHOW_INTEL_LLC_MISSES = 0

# - sandybridge performance counters
SHOW_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK = 0
SHOW_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK = 0
SHOW_INTEL_SANDYBRIDGE_L1C_HITS = 0
SHOW_INTEL_SANDYBRIDGE_L2C_HITS = 0
SHOW_INTEL_SANDYBRIDGE_ICACHE_MISSES = 0

#  ////////////////////////////////////////////////////////////////
# / Configuration section ends here. Don't change anything below /
#////////////////////////////////////////////////////////////////

PREDEFINES=

ifeq ($(ENABLE_HUGEPAGE),1)
    PREDEFINES += -DUSE_HUGEPAGE
endif

ifeq ($(TIMING_SERVICE),rdtsc)
    PREDEFINES += -DTIMING_WITH_RDTSC
else ifeq ($(TIMING_SERVICE),cpu_cycles)
    PREDEFINES += -DTIMING_WITH_CPU_CYCLES
else
    PREDEFINES += -DTIMING_WITH_CLOCK_GETTIME
endif

ifeq ($(SHOW_PERF_CPU_CYCLES),1)
    PREDEFINES += -DUSE_PERF_CPU_CYCLES
endif

ifeq ($(SHOW_PERF_SCHED_SWITCH),1)
    PREDEFINES += -DUSE_PERF_SCHED_SWITCH
    REQUIRE_PERF_TRACEPOINT_HEADER=1
    PREDEFINES += -DUSE_PERF_TRACEPOINT_HEADER
endif

ifeq ($(SHOW_INTEL_CPU_CYCLES),1)
    PREDEFINES += -DUSE_INTEL_CPU_CYCLES
endif

ifeq ($(SHOW_INTEL_REF_CYCLES),1)
    PREDEFINES += -DUSE_INTEL_REF_CYCLES
endif

ifeq ($(SHOW_INTEL_LLC_HITS),1)
    PREDEFINES += -DUSE_INTEL_LLC_HITS
endif

ifeq ($(SHOW_INTEL_LLC_MISSES),1)
    PREDEFINES += -DUSE_INTEL_LLC_MISSES
endif

ifeq ($(SHOW_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK),1)
    PREDEFINES += -DUSE_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK
endif

ifeq ($(SHOW_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK),1)
    PREDEFINES += -DUSE_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK
endif

ifeq ($(SHOW_INTEL_SANDYBRIDGE_L1C_HITS),1)
    PREDEFINES += -DUSE_INTEL_SANDYBRIDGE_L1C_HITS
endif

ifeq ($(SHOW_INTEL_SANDYBRIDGE_L2C_HITS),1)
    PREDEFINES += -DUSE_INTEL_SANDYBRIDGE_L2C_HITS
endif

ifeq ($(SHOW_SANDYBRIDGE_ICACHE_MISSES),1)
    PREDEFINES += -DUSE_SANDYBRIDGE_ICACHE_MISSES
endif
