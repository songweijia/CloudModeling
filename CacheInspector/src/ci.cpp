#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fstream>
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
#define OPT_SCHEDULE_FILE       "schedule_file"
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
    "\n2. Cache/Memory read latency test: --" OPT_READ_LAT "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_BUF_SIZE " <buffer size in KiB>\n" \
    "Optional arguments:\n" \
    "   [--" OPT_NUM_DPS " <number of data points,default is 32>]\n" \
    "   [--" OPT_TIMING_BY " <" OPT_TIMING_BY_GETTIME "|" OPT_TIMING_BY_RDTSC "|" OPT_TIMING_BY_CPUCYCLE ", default is " OPT_TIMING_BY_GETTIME ">]\n" \
    "   [--" OPT_SHOW_PERF "]\n" \
    "\n3. Cache/Memory throughput/latency test with schedule: --" OPT_SCHEDULE "\n" \
    "Compulsory arguments:\n" \
    "   --" OPT_SCHEDULE_FILE " <schedule file, please see default sample.schedule>\n" \
    "Optional arguments:\n" \
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
    {OPT_READ_LAT,          no_argument,0,0},
    {OPT_SCHEDULE,          no_argument,0,0},
    {OPT_CACHE_SIZE,        no_argument,0,0},
    {OPT_BUF_SIZE,          required_argument,0,0},
    {OPT_TOT_SIZE,          required_argument,0,0},
    {OPT_NUM_DPS,           required_argument,0,0},
    {OPT_SHOW_PERF,         no_argument,0,0},
    {OPT_SCHEDULE_FILE,     required_argument,0,0},
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
    char*       schedule_file = nullptr;
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
                      << "schedule_file:" << schedule_file << '\n'
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

static void show_perf_counters(const std::string& series_name, const double res[], const std::vector<std::map<std::string, long long>>& lpcs) {
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
        std::cout << std::left << std::setw(32) << res[idx++];
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

/**
 * Parse a schedule file to a schedule
 *
 * @param schedule_file The schedule file, please see sample.schedule for format description.
 * @param schedule      The output parameter to receive parsed schedule
 *
 * @return 0 for success, other for failure.
 */
int parse_schedule(const char* schedule_file, ci_schedule_t& schedule) {
    std::ifstream infile(schedule_file);
    if (!infile.is_open()) {
        std::cerr << "Failed to open schedul file for read:" << schedule_file << std::endl;
        return -1;
    }
    // reset the contents
    schedule.clear();
    // true
    while(1) {
        char parse_buffer[1024];
        ci_schedule_entry_t schedule_entry;
        uint64_t prev_buffer_size=0;
        if(!infile.getline(parse_buffer,1024)){
            break;
        }
        uint16_t pos = 0;
        // pos to the first char.
        while(pos<1024) {
            if (parse_buffer[pos] == ' ' || parse_buffer[pos] == '\t') {
                pos ++;
            } else {
                break;
            }
        }
        // skip empty lines or comments(started with #)
        if (pos == 1024 || parse_buffer[pos] == '\0' || parse_buffer[pos] == '#') {
            continue;
        }
        // find col 1: buffer_size
        schedule_entry.buffer_size = std::stoull(parse_buffer+pos,nullptr,0);
        if (schedule_entry.buffer_size < prev_buffer_size) {
            std::cerr << "line:'" << parse_buffer << "' has a buffer size smaller than previous one:" << prev_buffer_size << std::endl;
            return -1;
        }

///SYNTACTIC SUGAR///
#define NEXT_COL \
        while (pos < 1024) { \
            if (parse_buffer[pos] != ',' && parse_buffer[pos] != '\0') { \
                pos++; \
            } else { \
                break; \
            } \
        } \
        if (pos == 1024 || parse_buffer[pos] != ',') { \
            std::cerr << "Warning: skip invalid line:" << parse_buffer << std::endl; \
            continue; \
        } else { \
            pos ++; \
        }
/////////////////////

        // find col 2: enable_thp:=0|1
        NEXT_COL;
        schedule_entry.enable_thp = (std::stoul(parse_buffer+pos,nullptr,0)!=0);
        // find col 3: total_data_size
        NEXT_COL;
        schedule_entry.total_data_size = std::stoull(parse_buffer+pos,nullptr,0);
        // find col 4: thp_num_datapoints
        NEXT_COL;
        schedule_entry.thp_num_datapoints = std::stoull(parse_buffer+pos,nullptr,0);
        // find col 5: enable_lat:=0|1
        NEXT_COL;
        schedule_entry.enable_lat = (std::stoul(parse_buffer+pos,nullptr,0)!=0);
        // find col 6: thp_num_datapoints
        NEXT_COL;
        schedule_entry.lat_num_datapoints = std::stoull(parse_buffer+pos,nullptr,0);
        schedule.emplace_back(schedule_entry);
    }
    infile.close();
    return 0;
}

class ConsoleCollector : public ScheduleResultCollector {
    timing_mechanism_t  timing;
    bool                show_perf;
    const char*         thp_unit;
    const char*         lat_unit;
public:
    /** constructor **/
    ConsoleCollector(timing_mechanism_t _timing = CLOCK_GETTIME,
            bool _show_perf = false) : timing(_timing),show_perf(_show_perf) {
        thp_unit = "GiB/s";
        lat_unit = "nsecs";
        if (timing == PERF_CPU_CYCLE) {
           thp_unit = "byte/cycle(CPU)";
           lat_unit = "cycle(CPU)";
        } else if (timing == RDTSC) {
            thp_unit = "byte/cycle(TSC)";
            lat_unit = "cycle(TSC)";
        }
    }
    /** throughput collector **/
    virtual void collect_throughput(const bool is_write,
                                    const uint64_t buffer_size,
                                    const uint64_t num_datapoints,
                                    const double results[],
                                    const std::optional<std::vector<std::map<std::string, long long>>>& lpcs) override {
        const char* read_write = (is_write?"WRITE":"READ");
        fprintf(stdout,"\n%s %.3f %s std %.3f min %.3f max %.3f\n", read_write,
                average(num_datapoints, results), thp_unit, deviation(num_datapoints, results),
                minimum(num_datapoints, results), maximum(num_datapoints, results));
        if (show_perf) {
            show_perf_counters( std::string(read_write) + "_THP(" + thp_unit + ")", results, *lpcs);
        }
    }
    /** latency collector **/
    virtual void collect_latency(const uint64_t buffer_size,
                                 const uint64_t num_datapoints,
                                 const double results[],
                                 const std::optional<std::vector<std::map<std::string, long long>>>& lpcs) override{
        fprintf(stdout,"\nLATENCY %.3f %s std %.3f min %.3f max %.3f\n",
                average(num_datapoints, results), lat_unit, deviation(num_datapoints, results),
                minimum(num_datapoints, results), maximum(num_datapoints, results));
        if (show_perf) {
            show_perf_counters("latency (" + std::string(lat_unit) + ")", results, *lpcs);
        }
    }
};

static void run_schedule(const struct parsed_args& pargs) {
    ci_schedule_t schedule;

    if (pargs.schedule_file) {
        if(parse_schedule(pargs.schedule_file,schedule)!=0){
            std::cerr << "Parsing schedule file failed." << std::endl;
            return;
        }
    } else {
        std::cerr << "Plesae specify the schedule file by --" OPT_SCHEDULE_FILE "." << std::endl;
        return;
    }

    ConsoleCollector cc(pargs.timing_by,pargs.show_perf);

    if(ci_schedule(schedule, cc, pargs.timing_by, pargs.show_perf)) {
        std::cerr << "ci_schedule()" << " failed." << std::endl;
    }
}

static void run_read_lat(const struct parsed_args& pargs) {
    std::optional<std::vector<std::map<std::string, long long>>> lpcs = std::vector<std::map<std::string, long long>>();
    double res[pargs.num_datapoints];
    if (rand_latency(nullptr, pargs.buffer_size_kbytes<<10,pargs.num_datapoints,res,lpcs,pargs.timing_by) != 0){
        std::cerr << "rand_latency() failed." << std::endl;
        return;
    }

    const char* time_unit = "nsecs";
    if (pargs.timing_by == RDTSC) {
        time_unit = "TSC ticks";
    } else if (pargs.timing_by == PERF_CPU_CYCLE) {
        time_unit = "cpu cycles";
    }
    fprintf(stdout,"\nLATENCY %.3f %s std %.3f min %.3f max %.3f\n",
            average(pargs.num_datapoints, res), time_unit, deviation(pargs.num_datapoints, res),
            minimum(pargs.num_datapoints, res), maximum(pargs.num_datapoints, res));
    if (pargs.show_perf) {
        show_perf_counters("latency (" + std::string(time_unit) + ")", res, *lpcs);
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
                strcmp(long_options[option_index].name,OPT_SCHEDULE) == 0 ||
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
            } else if (strcmp(long_options[option_index].name,OPT_SCHEDULE_FILE) == 0) {
                pargs.schedule_file = optarg;
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
        case '?':
            std::cerr << "Invalid argument list. Please try '" << argv[0] << " --" OPT_HELP << "'" << std::endl;
            return -1;
        default:
            std::cerr << "unknown command encountered." << std::endl;
            return -1;
        }
    }
    if (pargs.cmd_name == nullptr || pargs.request_help) {
        std::cout << HELP_INFO << std::endl;
    } else if (strcmp(pargs.cmd_name, OPT_SEQ_THP) == 0) {
        run_seq_thp(pargs);
    } else if (strcmp(pargs.cmd_name, OPT_READ_LAT) == 0) {
        run_read_lat(pargs);
    } else if (strcmp(pargs.cmd_name, OPT_SEQ_THP_SCHEDULE) == 0) {
        run_schedule(pargs);
    } else {
        std::cout << pargs.cmd_name << " to be supported." << std::endl;
    }
    return 0;
}
