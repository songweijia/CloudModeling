#include <stdlib.h>

#include <ci/config.h>

#if USE_HUGEPAGE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <ci/cache_size.hpp>
#include <ci/seq_thp.hpp>
#include <ci/util.hpp>

#define BUFFER_ALIGNMENT (4096)
#define MEM_ALLOCATION (256ull << 20)
namespace cacheinspector {
static int32_t binary_search(
#if LOG_BINARY_SEARCH
        int32_t* bs_log,
        double* bs_log_thp,
        const int32_t tot_search_depth,
#endif  //LOG_BINARY_SEARCH
        const uint32_t seed_KiB,
        const double* target_thps,
        const int num_samples,
        const bool is_write,
        const timing_mechanism_t timing,
        const int32_t search_depth,
        void* workspace,  // a memory workspace with MEM_ALLOCATION 256MiB.
        uint32_t* output,
        const int32_t num_iter_per_sample,
        const uint64_t num_bytes_per_iter,
        const uint32_t LB = 0ul,
        const uint32_t UB = 0ul) {

    uint32_t lb = LB;
    uint32_t ub = UB;

    // done
    if(num_samples == 0)
        return 0;

    // get the middle target
    const double target_thp = target_thps[num_samples / 2];

    // search ...
    uint32_t pivot = seed_KiB;
    double thps[num_iter_per_sample];
    uint32_t ret = 0;

    int loop = search_depth;
#if LOG_BINARY_SEARCH
    bs_log[(tot_search_depth - loop) + (tot_search_depth + 1) * (num_samples / 2)] = pivot;
#endif  //LOG_BINARY_SEARCH
    std::optional<std::vector<std::map<std::string, long long>>> no_counters;
    if(pivot != 0) {
        while(loop--) {
            size_t buffer_size = ((size_t)pivot) << 10;
            ret = sequential_throughput(workspace, buffer_size,
                                        num_iter_per_sample, thps,
                                        no_counters,  // not using the counters.
                                        timing,
                                        is_write, num_bytes_per_iter);
            RETURN_ON_ERROR(ret, "sequential_throughput");
            double v = average(num_iter_per_sample, thps);
            uint32_t new_pivot = pivot;
            if(v < target_thp) {  // go smaller
                ub = pivot;
                new_pivot = (ub + lb) / 2;
            } else {  // go bigger
                lb = pivot;
                if(ub == 0)
                    new_pivot = pivot << 1;
                else
                    new_pivot = (ub + lb) / 2;
            }
            if(new_pivot == 0) break;
            if(pivot == new_pivot)
                break;
            else
                pivot = new_pivot;
#if LOG_BINARY_SEARCH
            bs_log_thp[(tot_search_depth - loop - 1) + (tot_search_depth + 1) * (num_samples / 2)] = v;
            bs_log[(tot_search_depth - loop) + (tot_search_depth + 1) * (num_samples / 2)] = pivot;
#endif  //LOG_BINARY_SEARCH
        }
    }
    output[num_samples / 2] = pivot;

    // search others
    uint32_t npivot = (lb + pivot) / 2;
    uint32_t nofst = 0;
    uint32_t nlen = num_samples / 2;
    uint32_t nlb = LB;
    uint32_t nub = pivot;
    ret = binary_search(
#if LOG_BINARY_SEARCH
            bs_log,
            bs_log_thp,
            tot_search_depth,
#endif  //LOG_BINARY_SEARCH
            npivot,
            target_thps + nofst, nlen, is_write, timing,
            search_depth - 1, workspace, output + nofst,
            num_iter_per_sample, num_bytes_per_iter, nlb, nub);
    RETURN_ON_ERROR(ret, "binary_search, upper half");

    npivot = UB ? (pivot + UB) / 2 : 2 * pivot;
    nofst = num_samples / 2 + 1;
    nlen = num_samples - num_samples / 2 - 1;
    nlb = pivot;
    nub = ub;
    ret = binary_search(
#if LOG_BINARY_SEARCH
            bs_log,
            bs_log_thp,
            tot_search_depth,
#endif  //LOG_BINARY_SEARCH
            npivot,
            target_thps + nofst, nlen, is_write, timing,
            search_depth - 1, workspace, output + nofst,
            num_iter_per_sample, num_bytes_per_iter, nlb, nub);
    RETURN_ON_ERROR(ret, "binary_search, lower half");

    return ret;
}

int eval_cache_size(
        const uint32_t cache_size_hint_KiB,
        const double upper_thp,
        const double lower_thp,
        uint32_t* css,
        const int num_samples,
        const bool is_write,
        const timing_mechanism_t timing,
        const int32_t search_depth,
        const uint32_t num_iter_per_sample,
        const uint64_t num_bytes_per_iter,
        void* buf,
        const uint64_t buf_size,
        const bool warm_up_cpu) {
    int i, ret;
    void* ws = buf;

    if (buf == nullptr) {
#if USE_HUGEPAGE
#define ADDR (void*)(0x8000000000000000UL)
#define PROTECTION (PROT_READ | PROT_WRITE )
#define FLAGS (MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT))
        ws = mmap(ADDR, MEM_ALLOCATION, PROTECTION, FLAGS, -1, 0);
        if(ws == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
#else
        ret = posix_memalign(&ws, BUFFER_ALIGNMENT, MEM_ALLOCATION);
        RETURN_ON_ERROR(ret, "posix_memalign()");
#endif
    } else if (buf_size < MEM_ALLOCATION) {
        ret = EINVAL;
        RETURN_ON_ERROR(ret, "buf_size should be at least 256MiB");
    }

    double thps[num_samples];
    for(i = 0; i < num_samples; i++)
        thps[i] = upper_thp - (upper_thp - lower_thp) * (i + 1) / (num_samples + 1);

#if LOG_BINARY_SEARCH
    const int num_log_entry = num_samples * (search_depth + 1);
    int32_t* bs_log = (int32_t*)malloc(num_log_entry * sizeof(int32_t));
    double* bs_log_thp = (double*)malloc(num_log_entry * sizeof(double));
    if(bs_log == nullptr || bs_log_thp == nullptr) {
        fprintf(stderr, "failed to allocate log entry.\n");
        return -1;
    }
    bzero((void*)bs_log, num_log_entry * sizeof(int32_t));
    bzero((void*)bs_log_thp, num_log_entry * sizeof(double));
#endif
    // before we do binary search, we warm up the buffer and cpu together.
    if (warm_up_cpu) {
        double thps[num_iter_per_sample];
        std::optional<std::vector<std::map<std::string, long long>>> no_counters;
        for (int i=0;i<5;i++) {
            ret = sequential_throughput(ws,MEM_ALLOCATION,
                                        num_iter_per_sample, thps,
                                        no_counters,  // not using the counters.
                                        timing,
                                        is_write, num_bytes_per_iter);
        }
    }

    ret = binary_search(
#if LOG_BINARY_SEARCH
            bs_log,
            bs_log_thp,
            search_depth,
#endif  //LOG_BINARY_SEARCH
            cache_size_hint_KiB, thps, num_samples,
            is_write, timing, search_depth, ws, css,
            num_iter_per_sample, num_bytes_per_iter);
    RETURN_ON_ERROR(ret, "binary_search");

#if LOG_BINARY_SEARCH
    printf("search log:\n");
    for(int i = 0; i < num_samples; i++) {
        printf("\t");
        for(int j = 0; j <= search_depth; j++)
            printf("(%d,%.2f), ", bs_log[i * (search_depth + 1) + j], bs_log_thp[i * (search_depth +1) + j]);
        printf("\n");
    }
#endif  //LOG_BINARY_SEARCH

    if (buf == nullptr) {
#if USE_HUGEPAGE
        munmap(ws, MEM_ALLOCATION);
#else
        free(ws);
#endif
    }

    return ret;
}

}//cacheinspector
