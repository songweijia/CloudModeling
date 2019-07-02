#ifndef LINUX_PERF_COUNTERS_HPP
#define LINUX_PERF_COUNTERS_HPP

#include <linux/perf_event.h>
#include <string>
#include <atomic>
#include <vector>
#include <map>

#if USE_PERF_TRACEPOINT_HEADER
#include "linux_perf_tracepoint_ids.hpp"
#endif

/**
 * Macros controlling which counters are used:
 * - USE_PERF_SCHED_SWITCH
 *
 * - USE_PERF_CPU_CYCLES
 * - USE_PERF_BUS_CYCLES
 * - USE_PERF_REF_CYCLES
 *
 * - USE_INTEL_CPU_CYCLES
 * - USE_INTEL_LLC_MISSES
 * - USE_INTEL_LLC_HITS
 * - USE_INTEL_SANDYBRIDGE_TLB_LOAD_MISS_CAUSES_A_PAGE_WALK
 * - USE_INTEL_SANDYBRIDGE_TLB_STORE_MISS_CAUSES_A_PAGE_WALK
 * - USE_INTEL_SANDYBRIDGE_L1C_HITS
 * - USE_INTEL_SANDYBRIDGE_L2C_HITS
 */

class LinuxPerfCounter {
public:
    // constructor
    LinuxPerfCounter(const char* name,
        uint32_t type,
        uint64_t config,
        bool exclude_kernel = true,
        bool exclude_hv = true);
    // moving constructor
    LinuxPerfCounter(LinuxPerfCounter&& lpc);
    // destructor
    virtual ~LinuxPerfCounter();
    // start record
    void start();
    // stop record
    void stop();
    // get counter value
    inline long long get();
    // get name of the counter
    const std::string getName();
    // give up ownership of the fd for moving constructor
    void giveup();
private:
    const std::string name;
    long long counter;
    struct perf_event_attr pe;
    int fd;
    std::atomic<bool> started;
};

class LinuxPerfCounters {
private:
    std::map<std::string,LinuxPerfCounter> counters;

public:
    LinuxPerfCounters();
    virtual ~LinuxPerfCounters();
    void start_perf_events();
    void stop_perf_events();
    std::map<std::string,long long> get();
    virtual void print(std::ostream& os, bool withMetadata = true);
};

#endif//LINUX_PERF_COUNTERS_HPP
