#include <iostream>
#include <iomanip>
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
#define OPT_TIMING_BY           "timing_by"
#define OPT_TIMING_BY_GETTIME   "clock_gettime"
#define OPT_TIMING_BY_RDTSC     "rdtsc"
#define OPT_TIMING_BY_CPUCYCLE  "cpu_cycle"
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
    "   [--" OPT_TIMING_BY " <" OPT_TIMING_BY_GETTIME "|" OPT_TIMING_BY_RDTSC "|" OPT_TIMING_BY_CPUCYCLE ", default is " OPT_TIMING_BY_GETTIME ">]\n" \
    "\n2. Cache/Memory throughput test with schedule: --" OPT_SEQ_THP_SCHEDULE "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_SCHEDULE " <schedule file, please see default sequential_throughput.schedule>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_TIMING_BY " <" OPT_TIMING_BY_GETTIME "|" OPT_TIMING_BY_RDTSC "|" OPT_TIMING_BY_CPUCYCLE ", default is " OPT_TIMING_BY_GETTIME ">]\n" \
    "\n3. Cache/Memory read latency test: --" OPT_READ_LAT "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_BUF_SIZE " <buffer size in KiB>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_NUM_DPS " <number of data points,default is 32>]\n" \
    "   [--" OPT_TIMING_BY " <" OPT_TIMING_BY_GETTIME "|" OPT_TIMING_BY_RDTSC "|" OPT_TIMING_BY_CPUCYCLE ", default is " OPT_TIMING_BY_GETTIME ">]\n" \
    "   [--" OPT_SHOW_PERF "]\n" \
    "\n4. Cache size test: --" OPT_CACHE_SIZE "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_FASTER_TIER_THP " <the throughput of the faster cache tier in GiB/s>\n" \
    "   --" OPT_SLOWER_TIER_THP " <the throughput of the slower cache tier in GiB/s>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_CACHE_SIZE_HINT " <the hint of the the cache size in KiB, default is 20480>]\n" \
    "   [--" OPT_NUM_DPS " <number of data points, default is 32>]\n" \
    "   [--" OPT_TIMING_BY " <" OPT_TIMING_BY_GETTIME "|" OPT_TIMING_BY_RDTSC "|" OPT_TIMING_BY_CPUCYCLE ", default is " OPT_TIMING_BY_GETTIME ">]\n" \
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
    {OPT_TIMING_BY,         required_argument,0,0},
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
    timing_mechanism_t  timing_by = CLOCK_GETTIME;
    bool        request_help = false;

    void set_timing_by(const char* timing_by_string) {
        if (strcmp(OPT_TIMING_BY_GETTIME,timing_by_string) == 0) {
            timing_by = CLOCK_GETTIME;
        } else if (strcmp(OPT_TIMING_BY_RDTSC,timing_by_string) == 0) {
            timing_by = RDTSC;
        } else if (strcmp(OPT_TIMING_BY_CPUCYCLE,timing_by_string) == 0) {
            timing_by = PERF_CPU_CYCLE;
        } else {
            // default
            timing_by = CLOCK_GETTIME;
        }
    }

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
                      << "cache_size_hint_kbytes:" << cache_size_hint_kbytes << '\n'
                      << "timing_by:" << timing_by << '\n'
                      << "request_help:" << request_help << std::endl;
        } else {
            std::cout << "This parsed_args is uninitialized." << std::endl;
        }
        
    }
};

static void show_perf_counters(const std::string& series_name, double res[], std::vector<std::map<std::string, long long>>& lpcs) {
    if (lpcs.empty() || lpcs[0].empty()) {
        return;
    }
    // titles
    std::cout << std::left << std::setw(32) << series_name;
    for (const auto& kv:lpcs[0]) {
        std::cout << std::left << std::setw(16) << kv.first;
    }
    std::cout << std::endl;
    // data points
    uint32_t idx = 0;
    for (const auto& row:lpcs) {
        std::cout << std::left << std::setw(32) << res[idx];
        for (const auto& kv:row) {
            std::cout << std::left << std::setw(16) << kv.second;
        }
        std::cout << std::endl;
    }
}

static void run_seq_thp(const struct parsed_args& pargs) {
    double res[pargs.num_datapoints];
    std::optional<std::vector<std::map<std::string, long long>>> lpcs = std::vector<std::map<std::string, long long>>();
    const char* thp_unit = "GiB/s";
    if (pargs.timing_by == PERF_CPU_CYCLE) {
       thp_unit = "byte/cycle(CPU)";
    } else if (pargs.timing_by == RDTSC) {
        thp_unit = "byte/cycle(TSC)";
    }
    //verify arguments
    if (strcmp(pargs.cmd_name,OPT_SEQ_THP)!=0) {
        std::cerr << "command:" << pargs.cmd_name << " is not " << OPT_SEQ_THP << std::endl;
        return;
    }
    if (pargs.buffer_size_kbytes == 0) {
        std::cerr << OPT_BUF_SIZE << " must be greater than 0." << std::endl;
        return;
    }
    if (pargs.num_datapoints == 0) {
        std::cerr << OPT_NUM_DPS << " must be greater than 0." << std::endl;
        return;
    }
    if (pargs.total_size_mbytes == 0) {
        std::cerr << OPT_TOT_SIZE << " must be greater than 0." << std::endl;
        return;
    }

    //warm up CPU
    boost_cpu();

    // read
    if (sequential_throughput(NULL,
                              pargs.buffer_size_kbytes<<10,
                              pargs.num_datapoints,
                              res,lpcs,pargs.timing_by,0,pargs.total_size_mbytes<<20)) {
        std::cerr << "read throughput test failed in " << __func__ << std::endl; 
        return;
    } else {
        fprintf(stdout,"\nREAD %.3f %s std %.3f min %.3f max %.3f\n",
                average(pargs.num_datapoints, res), thp_unit, deviation(pargs.num_datapoints, res),
                minimum(pargs.num_datapoints, res), maximum(pargs.num_datapoints, res));
    }
    if (pargs.show_perf) {
        show_perf_counters("read_thp(" + std::string(thp_unit) + ")", res, *lpcs);
    }
    // write
    lpcs = std::vector<std::map<std::string, long long>>();
    if (sequential_throughput(NULL,
                              pargs.buffer_size_kbytes<<10,
                              pargs.num_datapoints,
                              res,lpcs,pargs.timing_by,1,pargs.total_size_mbytes<<20)) {
        std::cerr << "write throughput test failed in " << __func__ << std::endl; 
        return;
    } else {
        fprintf(stdout,"\nWRITE %.3f %s std %.3f min %.3f max %.3f\n",
                average(pargs.num_datapoints, res), thp_unit, deviation(pargs.num_datapoints, res),
                minimum(pargs.num_datapoints, res), maximum(pargs.num_datapoints, res));
    }
    if (pargs.show_perf) {
        show_perf_counters("write_thp(" + std::string(thp_unit) + ")", res, *lpcs);
    }
}

int main(int argc, char** argv) {
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
            } else if (strcmp(long_options[option_index].name,OPT_TIMING_BY) == 0) {
                pargs.set_timing_by(optarg);
            } else {
                std::cerr << "Unknown argument:" << long_options[option_index].name << std::endl;
                std::cout << HELP_INFO << std::endl;
            }
            break;
        case 'h':
            pargs.request_help = true;
            break;
        default:
            std::cerr << "unknown command encountered." << std::endl;
        }
    }
    if (pargs.cmd_name == nullptr || pargs.request_help) {
        std::cout << HELP_INFO << std::endl;
    } else if (strcmp(pargs.cmd_name, OPT_SEQ_THP) == 0) {
        run_seq_thp(pargs);
    } else {
        std::cout << pargs.cmd_name << " to be supported." << std::endl;
    }
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
