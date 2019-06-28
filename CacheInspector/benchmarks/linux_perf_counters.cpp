#include "linux_perf_counters.hpp"
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

LinuxPerfCounter::LinuxPerfCounter(const char* name,
    uint32_t type,
    uint64_t config,
    bool exclude_kernel,
    bool exclude_hv):
    name(name) {

    counter = 0;
    started = false;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = type;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = config;
    pe.disabled = 1;
    pe.exclude_kernel = exclude_kernel?1:0;
    pe.exclude_hv = exclude_hv?1:0;
    
    fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        std::cerr << "Error opening event: " << name
                  << ", type=" << type
                  << ", config=0x" << std::hex << config
                  << std::dec << std::endl;
        exit(EXIT_FAILURE);
    }
}

LinuxPerfCounter::LinuxPerfCounter(LinuxPerfCounter&& rhs):
    name(rhs.name) {
    counter = rhs.counter;
    started = rhs.started.load();
    pe = rhs.pe;
    fd = rhs.fd;
    rhs.giveup();
}

LinuxPerfCounter::~LinuxPerfCounter() {
    stop();
    if (fd != -1) {
        close(fd);
    }
}

void LinuxPerfCounter::start() {
    if (!started) {
        if(ioctl(fd, PERF_EVENT_IOC_RESET, 0)) {
            std::cerr << "Error reset counter for event: " << name
                      << ", type=" << pe.type
                      << ", config=0x" << std::hex << pe.config
                      << std::dec << ", errno=" << errno
                      << std::endl;
            exit(EXIT_FAILURE);
        }
        if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0)) {
            std::cerr << "Error enable counter for event: " << name
                      << ", type=" << pe.type
                      << ", config=0x" << std::hex << pe.config
                      << std::dec << ", errno=" << errno
                      << std::endl;
            exit(EXIT_FAILURE);
        }
        this->counter = 0;
        started = true;
    }
}

void LinuxPerfCounter::stop() {
    if (started) {
        if (read(fd, &counter, sizeof(counter)) == -1) {
            std::cerr << "Error reading counter for event: " << name
                      << ", type=" << pe.type
                      << ", config=0x" << std::hex << pe.config
                      << std::dec << ", errno=" << errno
                      << std::endl;
        }
        if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0)) {
            std::cerr << "Error disable counter for event: " << name
                      << ", type=" << pe.type
                      << ", config=0x" << std::hex << pe.config
                      << std::dec << ", errno=" << errno
                      << std::endl;
            exit(EXIT_FAILURE);
        }
        started = false;
    }
}

long long LinuxPerfCounter::get() {
    return counter;
}

const std::string LinuxPerfCounter::getName() {
    return this->name;
}

void LinuxPerfCounter::giveup() {
    fd = -1;
}

LinuxPerfCounters::LinuxPerfCounters() {
#ifdef USE_PERF_SCHED_SWITCH
    counters.emplace_back("sched:sched_switch", PERF_TYPE_TRACEPOINT, SCHED_SCHED_SWITCH, false);
#endif

#ifdef USE_PERF_CPU_CYCLES
    counters.emplace_back("cpu_cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
#endif

#ifdef USE_PERF_BUS_CYCLES
    counters.emplace_back("bus_cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES);
#endif

#ifdef USE_PERF_REF_CYCLES
    counters.emplace_back("ref_cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_REF_CPU_CYCLES);
#endif

#ifdef  USE_INTEL_CPU_CYCLES
    counters.emplace_back("intel_cpu_cycles", PERF_TYPE_RAW, 0x003c);
#endif

#ifdef USE_INTEL_REF_CYCLES
    counters.emplace_back("intel_ref_cycles", PERF_TYPE_RAW, 0x013c);
#endif

#ifdef USE_INTEL_LLC_HITS
    counters.emplace_back("intel_llc_hits", PERF_TYPE_RAW, 0x4f2e);
#endif

#ifdef USE_INTEL_LLC_MISSES
    counters.emplace_back("intel_llc_misses", PERF_TYPE_RAW, 0x412e);
#endif

#ifdef USE_INTEL_SANDYBRIDGE_L1C_HITS
    counters.emplace_back("intel_sandybridge_l1c_hits", PERF_TYPE_RAW, 0x01d1);
#endif

#ifdef USE_INTEL_SANDYBRIDGE_L2C_HITS
    counters.emplace_back("intel_sandybridge_l2c_hits", PERF_TYPE_RAW, 0x02d1);
#endif

#ifdef USE_INTEL_SANDYBRIDGE_DTLB_LOAD_MISS_CAUSES_A_PAGE_WALK
    counters.emplace_back("intel_sandybridge_dtlb_load_miss_causes_a_page_walk", PERF_TYPE_RAW, 0x0108);
#endif

#ifdef USE_INTEL_SANDYBRIDGE_DTLB_STORE_MISS_CAUSES_A_PAGE_WALK
    counters.emplace_back("intel_sandybridge_dtlb_store_miss_causes_a_page_walk", PERF_TYPE_RAW, 0x0149);
#endif

#ifdef USE_INTEL_SANDYBRIDGE_ICACHE_MISSES
    counters.emplace_back("intel_sandybridge_icache_misses", PERF_TYPE_RAW, 0x0280);
#endif
}

LinuxPerfCounters::~LinuxPerfCounters() {
}

void LinuxPerfCounters::start_perf_events() {
    for(auto itr = counters.begin(); itr != counters.end(); itr++) {
        itr->start();
    }
}

void LinuxPerfCounters::stop_perf_events() {
    for(auto itr = counters.begin(); itr != counters.end(); itr++) {
        itr->stop();
    }
}

std::vector<std::string> LinuxPerfCounters::getMetadata() {
    std::vector<std::string> ret;
    for(auto itr = counters.begin(); itr != counters.end(); itr++) {
        ret.emplace_back(itr->getName());
    }
    return std::move(ret);
}

std::vector<long long> LinuxPerfCounters::getCounters() {
    std::vector<long long> ret;
    for (auto itr = counters.begin(); itr != counters.end(); itr++) {
        ret.emplace_back(itr->get());
    }
    return std::move(ret);
}
