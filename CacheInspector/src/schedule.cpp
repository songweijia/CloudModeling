#include <assert.h>
#include <errno.h>
#include <iostream>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <optional>
#include <unistd.h>

#include <ci/config.h>
#if USE_HUGEPAGE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#define ADDR (void*)(0x6000000000000000UL)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT))
#endif


#include <ci/linux_perf_counters.hpp>
#include <ci/seq_thp.hpp>
#include <ci/rand_lat.hpp>
#include <ci/schedule.hpp>
#include <ci/rdtsc.hpp>
#include <ci/util.hpp>

namespace cacheinspector {

extern volatile int32_t ci_schedule (
    const ci_schedule_t& schedule,
    ScheduleResultCollector& collector,
    timing_mechanism_t timing,
    bool show_perf_counters) {

#if !USE_PERF_CPU_CYCLES
    if (timing == PERF_CPU_CYCLE) {
        RETURN_ON_ERROR(-0xffff, "Please enable compilation option:USE_PERF_CPU_CYCLES to use cpu cycle timing.");
    }
#endif

#if !USE_INTEL_CPU_CYCLES
    if (timing == HW_CPU_CYCLE) {
        RETURN_ON_ERROR(-0xffff, "Please enable compilation option:USE_INTEL_CPU_CYCLES to use cpu cycle timing.");
    }
#endif

    // STEP 1: get parameters
    uint64_t len_schedule = schedule.size();
    if (schedule.empty()) {
        // empty schedule
        RETURN_ON_ERROR(-1,"Error: empty schedule.");
    }
    uint64_t max_buffer_size = schedule.back().buffer_size;
    void* buf = nullptr;
#if USE_HUGEPAGE
    const uint64_t page_size = (1ull<<21); // we use 2MB hugepage
#else
    const uint64_t page_size = getpagesize();
#endif
    max_buffer_size = (max_buffer_size + page_size - 1)/page_size*page_size;

    // STEP 2: allocate space
#if USE_HUGEPAGE
    buf = mmap(ADDR, max_buffer_size, PROTECTION, FLAGS, -1, 0);
    if(buf == MAP_FAILED) {
        perror("mmap");
        fprintf(stderr,"errno=%d\n", errno);
        return -2;
    }
#else
    if (posix_memalign(&buf, page_size, max_buffer_size)) {
        RETURN_ON_ERROR(-3, "posix_memalign.");
    }
#endif

    // STEP 3: do tests
    for (uint64_t i=0;i<len_schedule;i++) {
        // 3.1 - run throughput test
        uint64_t num_iter_warmup = schedule[i].thp_num_datapoints/6;
        if (num_iter_warmup == 0) num_iter_warmup = 1;
        if (num_iter_warmup > 5) num_iter_warmup = 5;
#define MAX(x,y) ((x)>(y)?(x):(y))
        double iter_results[MAX(schedule[i].thp_num_datapoints,schedule[i].lat_num_datapoints)];
        auto lpcs = std::optional<std::vector<std::map<std::string, long long>>>();
        if (show_perf_counters) {
            lpcs = std::vector<std::map<std::string, long long>>();
        }

        // read throughput
        if (sequential_throughput(buf,
                                  schedule[i].buffer_size,
                                  schedule[i].thp_num_datapoints,
                                  iter_results,
                                  lpcs,
                                  timing,
                                  0,
                                  schedule[i].total_data_size)) {
            RETURN_ON_ERROR(-4, "Read test with sequential_throughput() failed.");
        } else {
            collector.collect_throughput(false,
                                         schedule[i].buffer_size,
                                         schedule[i].thp_num_datapoints,
                                         iter_results,lpcs);
            if (lpcs) (*lpcs).clear();
        }
        // write throughput
        if (sequential_throughput(buf,
                                  schedule[i].buffer_size,
                                  schedule[i].thp_num_datapoints,
                                  iter_results,
                                  lpcs,
                                  timing,
                                  1,
                                  schedule[i].total_data_size)) {
            RETURN_ON_ERROR(-4, "Write test with sequential_throughput() failed.");
        } else {
            collector.collect_throughput(true,
                                         schedule[i].buffer_size,
                                         schedule[i].thp_num_datapoints,
                                         iter_results,lpcs);
            if (lpcs) (*lpcs).clear();
        }
        // 3.2 - run latency test
        if (rand_latency(buf,
                         schedule[i].buffer_size,
                         schedule[i].lat_num_datapoints,
                         iter_results,
                         lpcs,
                         timing)) {
            RETURN_ON_ERROR(-5, "Latency test with rand_latency() failed.");
        } else {
            collector.collect_latency(schedule[i].buffer_size,
                                      schedule[i].lat_num_datapoints,
                                      iter_results,lpcs);
            if (lpcs) (*lpcs).clear();
        }
    }

    // STEP 4: deallocate space
#if USE_HUGEPAGE
    munmap(buf, max_buffer_size);
#else
    free(buf);
#endif
    return 0;
}

}//namespace cacheinspector
