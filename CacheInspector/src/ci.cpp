#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ci/ci.hpp>

using namespace cacheinspector;

#define OPT_SEQ_THP             "sequential_throughput"
#define OPT_SEQ_THP_SCHEDULE    "sequential_throughput_schedule"
#define OPT_READ_LAT            "read_latency"
#define OPT_CACHE_SIZE          "cache_size"
#define OPT_BUF_SIZE            "buffer_size"
#define OPT_TOT_SIZE            "total_size"
#define OPT_NUM_DPS             "num_datapoints"
#define OPT_SHOW_PERF           "show_perf_counters"
#define OPT_SCHEDULE            "schedule"
#define OPT_FASTER_TIER_THP     "faster_throughput"
#define OPT_SLOWER_TIER_THP     "slower_throughput"
#define OPT_CACHE_SIZE_HINT     "cache_size_hint"
#define OPT_HELP                "help"

#define HELP_INFO   \
    "=== CacheInspector Usage ===\n" \
    "\n1. Cache/Memory throughput test: --" OPT_SEQ_THP "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_BUF_SIZE " <buffer size in KiB>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_TOT_SIZE " <total data size in MiB, default is 128MiB>]\n" \
    "   [--" OPT_NUM_DPS " <number of data points,default is 32>]\n" \
    "   [--" OPT_SHOW_PERF "]\n" \
    "\n2. Cache/Memory throughput test with schedule: --" OPT_SEQ_THP_SCHEDULE "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_SCHEDULE " <schedule file, please see default sequential_throughput.schedule>\n" \
    "\n3. Cache/Memory read latency test: --" OPT_READ_LAT "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_BUF_SIZE " <buffer size in KiB>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_NUM_DPS " <number of data points,default is 32>]\n" \
    "   [--" OPT_SHOW_PERF "]\n" \
    "\n4. Cache size test: --" OPT_CACHE_SIZE "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_FASTER_TIER_THP " <the throughput of the faster cache tier in GiB/s>\n" \
    "   --" OPT_SLOWER_TIER_THP " <the throughput of the slower cache tier in GiB/s>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_CACHE_SIZE_HINT " <the hint of the the cache size in KiB, default is 20480>]\n" \
    "   [--" OPT_NUM_DPS " <number of data points, default is 32>]\n" \
    "\n*. Print this message: --" OPT_HELP 

static struct option long_options[] = {
    {OPT_SEQ_THP,           no_argument,0,0},
    {OPT_SEQ_THP_SCHEDULE,  no_argument,0,0},
    {OPT_READ_LAT,          no_argument,0,0},
    {OPT_CACHE_SIZE,        no_argument,0,0},
    {OPT_BUF_SIZE,          required_argument,0,0},
    {OPT_TOT_SIZE,          required_argument,0,0},
    {OPT_NUM_DPS,           required_argument,0,0},
    {OPT_SHOW_PERF,         no_argument,0,0},
    {OPT_SCHEDULE,          required_argument,0,0},
    {OPT_FASTER_TIER_THP,   required_argument,0,0},
    {OPT_SLOWER_TIER_THP,   required_argument,0,0},
    {OPT_CACHE_SIZE_HINT,   required_argument,0,0},
    {OPT_HELP,              no_argument,0,'h'}
};

struct parsed_args {
    const char* cmd_name = nullptr;
    uint64_t    buffer_size_kbytes = 0;
    uint64_t    total_size_mbytes = 0;
    uint64_t    num_datapoints = 0;
    bool        show_perf = false;
    char*       schedule;
    double      faster_thp = -1.0f;
    double      slower_thp = -1.0f;
    uint64_t    cache_size_hint_kbytes = 0;

    void initialize(const char* cmd) {
        cmd_name = cmd;
        if (strcmp(cmd,OPT_SEQ_THP) == 0) {
            if (total_size_mbytes == 0)total_size_mbytes = 128;
            if (num_datapoints == 0)num_datapoints = 32;
        } else if (strcmp(cmd,OPT_READ_LAT) == 0) {
            if (num_datapoints == 0)num_datapoints = 32;
        } else if (strcmp(cmd,OPT_CACHE_SIZE) == 0) {
            if (num_datapoints == 0)num_datapoints = 32;
            if (cache_size_hint_kbytes == 0)cache_size_hint_kbytes = 20480;
        }
    }

    void dump() {
        if (cmd_name != nullptr) {
            std::cout << "cmd_name:" << cmd_name << '\n'
                      << "buffer_size_kbytes:" << buffer_size_kbytes << '\n'
                      << "total_size_mbytes:" << total_size_mbytes << '\n'
                      << "num_datapoints:" << num_datapoints << '\n'
                      << "show_perf:" << show_perf << '\n'
                      << "schedule:" << schedule << '\n'
                      << "faster_thp:" << faster_thp << '\n'
                      << "slower_thp:" << slower_thp << '\n'
                      << "cache_size_hint_kbytes:" << cache_size_hint_kbytes << std::endl;
        } else {
            std::cout << "This parsed_args is uninitialized." << std::endl;
        }
        
    }
};

int main(int argc, char** argv) {
    std::cout << HELP_INFO << std::endl;
    struct parsed_args pargs;
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h", long_options, &option_index);
        if (c == -1) break;
        switch(c) {
        case 0:
            if (strcmp(long_options[option_index].name,OPT_SEQ_THP) == 0 ||
                strcmp(long_options[option_index].name,OPT_SEQ_THP_SCHEDULE) == 0 ||
                strcmp(long_options[option_index].name,OPT_CACHE_SIZE) == 0 ||
                strcmp(long_options[option_index].name,OPT_READ_LAT) == 0) {
                if (pargs.cmd_name != nullptr) {
                    std::cerr << "Error:expect only one command but saw multiple:"
                              << pargs.cmd_name << " and "<< long_options[option_index].name
                              << std::endl;
                    return -1;
                } else {
                    pargs.initialize(long_options[option_index].name);
                }
            } else if (strcmp(long_options[option_index].name,OPT_BUF_SIZE) == 0) {
                pargs.buffer_size_kbytes = std::stoull(optarg);
            } else if (strcmp(long_options[option_index].name,OPT_TOT_SIZE) == 0) {
                pargs.total_size_mbytes= std::stoull(optarg);
            } else if (strcmp(long_options[option_index].name,OPT_NUM_DPS) == 0) {
                pargs.num_datapoints = std::stoull(optarg);
            } else if (strcmp(long_options[option_index].name,OPT_SHOW_PERF) == 0) {
                pargs.show_perf = true;
            } else if (strcmp(long_options[option_index].name,OPT_SCHEDULE) == 0) {
                pargs.schedule = optarg;
            } else if (strcmp(long_options[option_index].name,OPT_FASTER_TIER_THP) == 0) {
                pargs.faster_thp= std::stod(optarg);
            } else if (strcmp(long_options[option_index].name,OPT_SLOWER_TIER_THP) == 0) {
                pargs.slower_thp= std::stod(optarg);
            } else if (strcmp(long_options[option_index].name,OPT_CACHE_SIZE_HINT) == 0) {
                pargs.cache_size_hint_kbytes= std::stoull(optarg);
            } else {
                std::cerr << "Unknown argument:" << long_options[option_index].name << std::endl;
                std::cout << HELP_INFO << std::endl;
            }
            break;
        case 'h':
            std::cout << HELP_INFO << std::endl;
            break;
        default:
            std::cerr << "unknown command encountered." << std::endl;
        }
    }
    pargs.dump();
/**
    sequential_throughput_test_result_t result; 

    if(sequential_throughput_cliffs(default_sequential_throughput_test_schedule,result)) {
        std::cerr << "profiling cliff failed" << std::endl;
        return -1;
    }

    for(auto fourtuple: result) {
        std::cout << std::get<0>(fourtuple) << " "
                  << std::get<1>(fourtuple) << " "
                  << std::get<2>(fourtuple) << " "
                  << std::get<3>(fourtuple) << std::endl;
    }
**/
    return 0;
}
