include config.mk

CFLAGS=-I. -Ibenchmarks -O3
CPPFLAGS=-I. -Ibenchmarks -O3 --std=c++1z

CFLAGS=-I. -Ibenchmarks -O3 $(PREDEFINES)
CPPFLAGS=-I. -Ibenchmarks -O3 --std=c++1z $(PREDEFINES)
LDFLAGS=
LDLIBS=
SRCS=ci.cpp benchmarks/seq_thp.cpp benchmarks/cache_size.cpp benchmarks/rand_lat.cpp benchmarks/linux_perf_counters.cpp
HEADERS=benchmarks/seq_thp.hpp util.hpp benchmarks/linux_perf_counters.hpp
TRACEPOINT_HEADER=benchmarks/linux_perf_tracepoint_ids.hpp
PROGS=ci

ifeq ($(REQUIRE_PERF_TRACEPOINT_HEADER),1)
    HEADERS += $(TRACEPOINT_HEADER)
endif

all: $(PROGS) 

#$(ASMCODE): asm/seq_thp.cpp
#	g++ $(CPPFLAGS) -S -o $@ $^

${PROGS}: ${SRCS} $(HEADERS)

$(TRACEPOINT_HEADER):
	cd benchmarks && ./generate-tracepoint-ids-header.sh

clean:
	rm -rf ${PROGS} $(ASMCODE) $(TRACEPOINT_HEADER)
