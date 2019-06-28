CFLAGS=-I. -Ibenchmarks -O3 -DUSE_HUGEPAGE -DUSE_PERF_CPU_CYCLES -DUSE_PERF_SCHED_SWITCH
CPPFLAGS=-I. -Ibenchmarks -O3 --std=c++1z -DUSE_HUGEPAGE -DTIMING_WITH_CPU_CYCLES
# CPPFLAGS=-I. -Ibenchmarks -O3 --std=c++1z -DUSE_HUGEPAGE -DUSE_INTEL_REF_CYCLES -DUSE_INTEL_LLC_MISSES -DUSE_INTEL_LLC_HITS -DUSE_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK -DUSE_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK -DUSE_INTEL_SANDYBRIDGE_L1C_HITS -DUSE_INTEL_SANDYBRIDGE_L2C_HITS -DUSE_INTEL_SANDYBRIDGE_ICACHE_MISSES
# CPPFLAGS=-I. -Ibenchmarks -O0 -ggdb --std=c++11 -DLOG_BINARY_SEARCH
# CPPFLAGS=-I. -Ibenchmarks -O0 -ggdb --std=c++11
LDFLAGS=
LDLIBS=
SRCS=ci.cpp benchmarks/seq_thp.cpp benchmarks/cache_size.cpp benchmarks/rand_lat.cpp benchmarks/linux_perf_counters.cpp
HEADERS=benchmarks/seq_thp.hpp util.hpp benchmarks/linux_perf_counters.hpp benchmarks/linux_perf_tracepoint_ids.hpp
PROGS=ci

all: $(PROGS) 

#$(ASMCODE): asm/seq_thp.cpp
#	g++ $(CPPFLAGS) -S -o $@ $^

${PROGS}: ${SRCS} 

clean:
	rm -rf ${PROGS} ${ASMCODE}
