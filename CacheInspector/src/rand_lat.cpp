#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <malloc.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>

#include <ci/config.h>
#if USE_HUGEPAGE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#define ADDR (void*)(0x6000000000000000UL)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT))
#endif//USE_HUGEPAGE
#include <ci/util.hpp>
#include <ci/rdtsc.hpp>
#include <ci/rand_lat.hpp>
#include <ci/linux_perf_counters.hpp>



static uint64_t rx = 123456789L;
static uint64_t ry = 362436069L;
static uint64_t rz = 521288629L;

static uint64_t xorshf96() {
    rx ^= rx << 16;
    rx ^= rx >> 5;
    rx ^= rx << 1;

    uint64_t t = rx;
    rx = ry;
    ry = rz;
    rz = t ^ rx ^ ry;

    return rz;
}

static bool initialize_random_seed() {
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        fprintf(stderr, "clock_gettime() failed.\n");
        return false;
    }
    uint32_t nloop = ((uint32_t)ts.tv_nsec) & 0xffff;
    while(nloop--)
        xorshf96();
    return true;
}

static bool fill_cyclic_linked_list(uint64_t* cll, int64_t ecnt) {
    uint8_t* bytemap = (uint8_t*)malloc(ecnt);
    if(!bytemap) {
        fprintf(stderr, "failed to get byte map.\n");
        return false;
    }

    bzero((void*)bytemap, ecnt);
    if(!initialize_random_seed()) {
        return false;
    }

    int64_t head = xorshf96() % ecnt;
    int64_t offset = head;
    bytemap[offset] = 1;
    int64_t filled = 1;
    while(filled < ecnt) {
        int64_t next = xorshf96() % ecnt;
        while(bytemap[next]) next = (next + 1) % ecnt;
        cll[offset] = (uint64_t)&cll[next];
        offset = next;
        bytemap[offset] = 1;
        filled++;
    }
    cll[offset] = (uint64_t)&cll[head];

    free(bytemap);

    return true;
}

static double traverse_cyclic_linked_list(int64_t num, uint64_t* cll,
                                          std::optional<std::vector<std::map<std::string, long long>>>& counters,
                                          timing_mechanism_t timing) {
    LinuxPerfCounters lpcs;
    lpcs.start_perf_events();

    struct timespec clk_t1, clk_t2;
    uint64_t tsc_t1 = 0, tsc_t2 = 0;

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

    if (timing == CLOCK_GETTIME) {
        if(clock_gettime(CLOCK_MONOTONIC, &clk_t1) < 0) {
            fprintf(stderr, "failed to call clock_gettime().\n");
            return 0.0f;
        }
    } else if (timing == RDTSC) {
        tsc_t1 = rdtsc();
    }
    // STEP 1 - traverse
    __asm__ volatile(
            "movq	%0, %%rax \n\t"
            "movq	%1, %%rbx \n\t"
            "begin_traverse%=: \n\t"
            //load data
            "subq	$128, %%rax \n\t"
            "js		done_traverse%= \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "movq	(%%rbx), %%rbx \n\t"
            "jmp	begin_traverse%=\n\t"
            "done_traverse%=: \n\t"
            :  // no output
            : "m"(num), "m"(cll)
            : "rax", "rbx");

    if (timing == CLOCK_GETTIME) {
        if(clock_gettime(CLOCK_MONOTONIC, &clk_t2) < 0) {
            fprintf(stderr, "failed to call clock_gettime().\n");
            return 0.0f;
        }
    } else if (timing == RDTSC) {
        tsc_t2 = rdtsc();
    }

    lpcs.stop_perf_events();

    if(counters.has_value()) {
        counters->emplace_back(lpcs.get());
    }

    double ret = 0.0f;
    if (timing == CLOCK_GETTIME) {
        ret = static_cast<double>(clk_t2.tv_sec - clk_t1.tv_sec) * 1e9 + static_cast<double>(clk_t2.tv_nsec - clk_t1.tv_nsec);
    } else if (timing == RDTSC) {
        ret = static_cast<double>(tsc_t2 - tsc_t1);
    } else if (timing == PERF_CPU_CYCLE) {
        ret = static_cast<double>(lpcs.get().at("cpu_cycles(pf)"));
    } else if (timing == HW_CPU_CYCLE) {
        ret = static_cast<double>(lpcs.get().at("cpu_cycles(hw)"));
    } else {
        std::cerr << "timing mechanism is unknown:" << timing << std::endl;
        ret = 0.0;
    }

    return ret;
}

int32_t rand_latency(void* buffer,
                     int64_t buffer_size, 
                     int num_points, 
                     double* output, 
                     std::optional<std::vector<std::map<std::string, long long>>>& counters,
                     timing_mechanism_t timing) {
    // STEP 1 - prepare the cyclic linked list
    uint64_t* cll = static_cast<uint64_t*>(buffer);
    int64_t num_entries = buffer_size / sizeof(uint64_t);
    if (cll == nullptr) {
#if USE_HUGEPAGE
        cll = static_cast<uint64_t*>(mmap(ADDR, buffer_size, PROTECTION, FLAGS, -1, 0));
        if (cll == MAP_FAILED) {
            perror("mmap");
            fprintf(stderr,"errno=%d\n", errno);
            return -1;
        }
#else
        if(posix_memalign((void**)&cll, 4096, buffer_size) != 0) {
            fprintf(stderr, "fail to call posix_memalign. errno=%d\n", errno);
        }
#endif
    }
    if(fill_cyclic_linked_list(cll, num_entries) == false) {
        fprintf(stderr, "failed to fill the cyclic linked list.\n");
    }

    // STEP 2 - disable scheduler
    int max_pri = sched_get_priority_max(SCHED_FIFO);
    if(max_pri < 0) {
        fprintf(stderr, "sched_get_priority_max() failed. errno=%d\n", errno);
    }
    struct sched_param sch_parm;
    sch_parm.sched_priority = max_pri;
    if(sched_setscheduler(0, SCHED_FIFO, &sch_parm) != 0) {
        fprintf(stderr, "sched_setscheduler() failed.\n");
    }

    // STEP 3 - warm_up by scan the buffer 5 times.
    for(int l = 0; l < 5; l++) {
        for(int i = 0; i < num_entries; i += 8) {
#if(__cplusplus - 0) >= 201703L
            volatile uint64_t r1,r2,r3,r4;
#else
            volatile register uint64_t r1,r2,r3,r4;
#endif
            r1 = cll[i];
            r2 = cll[i+1];
            r3 = cll[i+2];
            r4 = cll[i+3];
            r1 = r2; //access rx to suppress the 'unused variable' warning.
            r2 = r3;
            r3 = r4;
            r4 = r1;
        }
    }

// STEP 4 - test
#define NUMBER_OF_ACCESS (16 << 10)
    for(int i = 0; i < num_points; i++)
        output[i] = traverse_cyclic_linked_list(NUMBER_OF_ACCESS, cll + (i % num_entries), counters, timing) / NUMBER_OF_ACCESS;

    // STEP 5 - clean up
    if (buffer == nullptr) {
#if USE_HUGEPAGE
        munmap(cll,buffer_size);
#else
        free(cll);
#endif
    }
    return 0;
}
