#include <assert.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cache_size.hpp"
#include "rand_lat.hpp"
#include "seq_thp.hpp"
#include "util.hpp"

#define HELP_INFO                                                                  \
    "CacheInspector benchmark\n"                                                   \
    "--(e)xperiment throughput|latency|cachesize\n"                                \
    "--buffer_(s)ize <size in KiB>\n"                                               \
    "--(n)um_of_datapoints <num>\n"                                                \
    "--batch_(S)ize <size in MiB>\n"                                                \
    "--(u)pper_read_thp (unit in bytes/cycle or GiB/s, based on timing facility)\n" \
    "--(l)ower_read_thp (unit in bytes/cycle or GiB/s, based on timing facility)\n" \
    "--(c)ache_size_hint_KiB <20480>\n"                                             \
    "--search_(d)epth <11>\n"                                                      \
    "--(N)um_of_thp_dps_per_binary_search <5>\n"                                   \
    "--(L)oop <1> \n"                                                              \
    "--show (p)erf counters\n"                                                     \
    "--(h)elp\n"

const struct option opts[] = {
        {"experiment", required_argument, 0, 'e'},
        {"buffer_size", required_argument, 0, 's'},
        {"num_of_datapoints", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {"batch_size", required_argument, 0, 'S'},
        {"upper_read_thp", required_argument, 0, 'u'},
        {"lower_read_thp", required_argument, 0, 'l'},
        {"cache_size_hint_KiB", required_argument, 0, 'c'},
        {"search_depth", required_argument, 0, 'd'},
        {"Loop", required_argument, 0, 'L'},
        {"Num_of_thp_dps_per_binary_search", required_argument, 0, 'N'},
        {"perf", no_argument, 0, 'p'},
        {0, 0, 0, 0}};

enum CMExp {
    EXP_HELP = 0,
    EXP_THROUGHPUT = 1,
    EXP_LATENCY = 2,
    EXP_CACHESIZE
};

static inline void print_timestamp() {
    struct timespec tv;
    if(clock_gettime(CLOCK_REALTIME, &tv) != 0) {
        fprintf(stderr, "clock_gettime() failed.\n");
    } else {
        fprintf(stdout, "%ld.%03ld \n", tv.tv_sec, tv.tv_nsec / 1000000);
    }
}

int do_throughput(int buffer_size_KiB, int num_iter, int batch_size_MiB, bool show_perf_counters) {
    double* res = (double*)malloc(sizeof(double) * num_iter);

#if defined(TIMING_WITH_CPU_CYCLES)
    const char* thp_unit = "byte/cycle(CPU)";
#elif defined(TIMING_WITH_RDTSC)
    const char* thp_unit = "byte/cycle(TSC)";
#elif defined(TIMING_WITH_CLOCK_GETTIME)
    const char* thp_unit = "GiB/s";
#endif

    // boost_cpu
    boost_cpu();

    std::optional<std::vector<std::map<std::string, long long>>> lpcs = std::vector<std::map<std::string, long long>>();
    // read
    if(sequential_throughput(NULL, ((size_t)buffer_size_KiB) << 10,
                             num_iter, res,
                             lpcs,
                             0, ((uint64_t)batch_size_MiB) << 20)) {
        fprintf(stderr, "experiment failed...\n");
    }

    printf("READ %.3f %s std %.3f min %.3f max %.3f\n",
           average(num_iter, res), thp_unit, deviation(num_iter, res),
           minimum(num_iter, res), maximum(num_iter, res));

    std::cout << std::left << std::setw(32) << "throughput per iter";
    if(lpcs.has_value() && show_perf_counters) {
        for(auto itr = (*lpcs)[0].begin(); itr != (*lpcs)[0].end(); itr++) {
            std::cout << std::left << std::setw(16) << itr->first;
        }
    }
    printf("\n");

    for(int i = 0; i < num_iter; i++) {
        printf("[%d]-%.3f %s\t", i, res[i], thp_unit);
        if(lpcs.has_value() && show_perf_counters) {
            for(auto itr = (*lpcs)[i].begin(); itr != (*lpcs)[i].end(); itr++) {
                std::cout << std::left << std::setw(16) << itr->second;
            }
        }
        printf("\n");
    }

    // write
    lpcs->clear();
    if(sequential_throughput(NULL, ((size_t)buffer_size_KiB) << 10,
                             num_iter, res,
                             lpcs,
                             1, ((uint64_t)batch_size_MiB) << 20)) {
        fprintf(stderr, "experiment failed...\n");
    }

    printf("WRITE %.3f %s std %.3f min %.3f max %.3f\n",
           average(num_iter, res), thp_unit, deviation(num_iter, res),
           minimum(num_iter, res), maximum(num_iter, res));

    std::cout << std::left << std::setw(32) << "thoughput per iter";
    if(lpcs.has_value() && show_perf_counters) {
        for(auto itr = (*lpcs)[0].begin(); itr != (*lpcs)[0].end(); itr++) {
            std::cout << std::left << std::setw(16) << itr->first;
        }
    }
    printf("\n");
    for(int i = 0; i < num_iter; i++) {
        printf("[%d]-%.3f %s\t", i, res[i], thp_unit);
        if(lpcs.has_value() && show_perf_counters) {
            for(auto itr = (*lpcs)[i].begin(); itr != (*lpcs)[i].end(); itr++) {
                std::cout << std::left << std::setw(16) << itr->second;
            }
        }
        printf("\n");
    }
}

int do_cachesize(const uint32_t cache_size_hint_KiB,
                 const double upper_read_thp,
                 const double lower_read_thp,
                 const int num_data_points,
                 const int batch_size,
                 const bool is_write,
                 const int32_t search_depth,
                 const int32_t num_of_thp_dps_per_binary_search) {
    uint32_t css[num_data_points];
    assert(upper_read_thp > 0.0);
    assert(lower_read_thp > 0.0);
    assert(num_data_points > 0);

    int ret = eval_cache_size(cache_size_hint_KiB, upper_read_thp, lower_read_thp, css, num_data_points, is_write, search_depth, num_of_thp_dps_per_binary_search, ((uint64_t)batch_size) << 20);
    if(ret) {
        fprintf(stderr, "failed...\n");
        return ret;
    }
    for(int i = 0; i < num_data_points; i++) {
        printf("[%d]\t%dKiB\n", i, css[i]);
    }
    return ret;
}

int do_latency(const int buffer_size_KiB,
               const int num_datapoints,
               const bool show_perf_counters) {
    double* latencies = (double*)malloc(sizeof(double) * num_datapoints);
    std::optional<std::vector<std::map<std::string, long long>>> lpcs = std::vector<std::map<std::string, long long>>();

    random_latency(((int64_t)buffer_size_KiB << 10), num_datapoints, latencies, lpcs);

#if defined(TIMING_WITH_CPU_CYCLES)
    const char* lat_unit = "cycles(CPU)";
#elif defined(TIMING_WITH_RDTSC)
    const char* lat_unit = "cycles(TSC)";
#elif defined(TIMING_WITH_CLOCK_GETTIME)
    const char* lat_unit = "nanosecond";
#endif

    std::cout << std::left << std::setw(32) << "Read latency";
    if(lpcs.has_value() && show_perf_counters) {
        for(auto itr = (*lpcs)[0].begin(); itr != (*lpcs)[0].end(); itr++) {
            std::cout << std::left << std::setw(16) << itr->first;
        }
    }
    printf("\n");

    for(int i = 0; i < num_datapoints; i++) {
        printf("[%d]-%.3f %s\t\t", i, latencies[i], lat_unit);
        if(lpcs.has_value() && show_perf_counters) {
            for(auto itr = (*lpcs)[i].begin(); itr != (*lpcs)[i].end(); itr++) {
                std::cout << std::left << std::setw(16) << itr->second;
            }
        }
        printf("\n");
    }

    free((void*)latencies);
}

int main(int argc, char** argv) {
    int c;
    int option_index = 0;
    int buffer_size_KiB = 0;
    int num_datapoints = 0;
    int batch_size_MiB = (1 << 8);
    enum CMExp exp = EXP_HELP;
    double upper_read_thp = 0.0f;
    double lower_read_thp = 0.0f;
    uint32_t cache_size_hint_KiB = 10240;
    int32_t search_depth = 11;
    int32_t num_of_thp_dps_per_binary_search = 5;
    int nloop = 1;
    bool show_perf_counters = false;
    // parse arguments.
    while(1) {
        c = getopt_long(argc, argv, "e:s:S:n:l:u:c:d:N:L:hp", opts, &option_index);
        if(c == -1)
            break;

        switch(c) {
            case 'e':
                if(strcmp(optarg, "throughput") == 0)
                    exp = EXP_THROUGHPUT;
                else if(strcmp(optarg, "latency") == 0)
                    exp = EXP_LATENCY;
                else if(strcmp(optarg, "cachesize") == 0)
                    exp = EXP_CACHESIZE;
                else
                    exp = EXP_HELP;
                break;

            case 's':
                buffer_size_KiB = atoi(optarg);
                break;

            case 'L':
                nloop = atoi(optarg);
                break;

            case 'n':
                num_datapoints = atoi(optarg);
                break;

            case 'S':
                batch_size_MiB = atoi(optarg);
                break;

            case 'l':
                lower_read_thp = atof(optarg);
                break;

            case 'u':
                upper_read_thp = atof(optarg);
                break;

            case 'c':
                cache_size_hint_KiB = (uint32_t)atoi(optarg);
                break;

            case 'd':
                search_depth = (int32_t)atoi(optarg);
                break;

            case 'N':
                num_of_thp_dps_per_binary_search = (int32_t)atoi(optarg);
                break;

            case 'p':
                show_perf_counters = true;
                break;

            default:
                printf("skip unknown opt code:0%o ??\n", c);
        }
    }

    switch(exp) {
        case EXP_HELP:
            printf("%s", HELP_INFO);
            break;

        case EXP_THROUGHPUT:
            fprintf(stderr, "=== Throughput Test ===\n");
            fprintf(stderr, "nloop:\t%d\n", nloop);
            fprintf(stderr, "buffer_size_KiB:\t%d\n", buffer_size_KiB);
            fprintf(stderr, "num_datapoints:\t%d\n", num_datapoints);
            fprintf(stderr, "batch_size_MiB:\t%d\n", batch_size_MiB);
            while(nloop--)
                do_throughput(buffer_size_KiB, num_datapoints, batch_size_MiB, show_perf_counters);
            break;

        case EXP_CACHESIZE:
            fprintf(stderr, "=== Cache Size Test ===\n");
            fprintf(stderr, "nloop:\t%d\n", nloop);
            fprintf(stderr, "cache_size_hint_KiB:\t%d\n", cache_size_hint_KiB);
            fprintf(stderr, "upper_read_thp:\t%f\n", upper_read_thp);
            fprintf(stderr, "lower_read_thp:\t%f\n", lower_read_thp);
            fprintf(stderr, "num_datapoints:\t%d\n", num_datapoints);
            fprintf(stderr, "batch_size_MiB:\t%d\n", batch_size_MiB);
            fprintf(stderr, "search_depth:\t%d\n", search_depth);
            fprintf(stderr, "num_of_thp_dps_per_binary_search:\t%d\n", num_of_thp_dps_per_binary_search);
            while(nloop--) {
                // print_timestamp();
                do_cachesize(cache_size_hint_KiB, upper_read_thp, lower_read_thp, num_datapoints, batch_size_MiB, false, search_depth, num_of_thp_dps_per_binary_search);
            }
            break;

        case EXP_LATENCY:
            fprintf(stderr, "=== Latency Test ===\n");
            fprintf(stderr, "nloop:\t%d\n", nloop);
            fprintf(stderr, "buffer_size_KiB:\t%d\n", buffer_size_KiB);
            fprintf(stderr, "num_datapoints:\t%d\n", num_datapoints);
            while(nloop--) {
                do_latency(buffer_size_KiB, num_datapoints, show_perf_counters);
            }
            break;
        default:;
    }

    return 0;
}
