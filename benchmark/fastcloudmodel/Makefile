CFLAGS=-I. -Ibenchmarks -O3
CPPFLAGS=-I. -Ibenchmarks -O3 --std=c++11
# CPPFLAGS=-I. -Ibenchmarks -O0 -ggdb --std=c++11 -DLOG_BINARY_SEARCH
# CPPFLAGS=-I. -Ibenchmarks -O0 -ggdb --std=c++11
LDFLAGS=
LDLIBS=
SRCS=cm.cpp benchmarks/seq_thp.cpp benchmarks/cache_size.cpp benchmarks/rand_lat.cpp
HEADERS=benchmarks/seq_thp.hpp util.hpp
PROGS=cm

all: $(PROGS) 

#$(ASMCODE): asm/seq_thp.cpp
#	g++ $(CPPFLAGS) -S -o $@ $^

${PROGS}: ${SRCS} 

clean:
	rm -rf ${PROGS} ${ASMCODE}
