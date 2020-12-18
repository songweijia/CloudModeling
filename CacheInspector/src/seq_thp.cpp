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
#include <ci/rdtsc.hpp>
#include <ci/util.hpp>

namespace cacheinspector {
extern int32_t volatile sequential_throughput(
        void* buffer,
        size_t buffer_size,
        uint32_t num_iter,
        double* results,
        std::optional<std::vector<std::map<std::string, long long>>>& counters,
        timing_mechanism_t timing,
        uint32_t is_write,
        uint64_t bytes_per_iter,
        uint64_t num_iter_warmup,
        size_t buf_alignment) {
    assert(buffer_size > 0);

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

    int ret = 0;
    struct timespec clk_ts, clk_te;
    uint64_t tsc_ts, tsc_te;
    uint64_t iter_per_iter = (bytes_per_iter + buffer_size - 1) / buffer_size;

    // STEP 1 - validate/allocate the buffer
    void* buf = buffer;
    if(buf == NULL) {
#if USE_HUGEPAGE
        buf = mmap(ADDR, buffer_size, PROTECTION, FLAGS, -1, 0);
        if(buf == MAP_FAILED) {
            perror("mmap");
            printf("errno=%d\n", errno);
            exit(1);
        }
#else
        ret = posix_memalign(&buf, buf_alignment, buffer_size);
        RETURN_ON_ERROR(ret, "posix_memalign.");
#endif
    }

    // STEP 2 - change scheduler
    int max_pri = sched_get_priority_max(SCHED_FIFO);
    RETURN_ON_ERROR(max_pri, "sched_get_priority_max.");
    struct sched_param sch_parm;
    sch_parm.sched_priority = max_pri;
    ret = sched_setscheduler(0, SCHED_FIFO, &sch_parm);
    RETURN_ON_ERROR(ret, "sched_setscheduler.");

    // STEP 3 - warm up
    while(num_iter_warmup--) {
        memset(buf,0,buffer_size);
    }

    // linux perf counters
    for(size_t iter = 0; iter < num_iter; iter++) {
        LinuxPerfCounters lpcs;

        // STEP 4 - start the timer
        if (timing == CLOCK_GETTIME) {
            ret = clock_gettime(CLOCK_MONOTONIC, &clk_ts);
            RETURN_ON_ERROR(ret, "clock_gettime");
        } else if (timing == RDTSC) {
            tsc_ts = rdtsc();
        }

        lpcs.start_perf_events();

        // STEP 5 - run experiment
        if(is_write) {
#if defined(__x86_64__)
            // -- write
            __asm__ volatile(
                    // r0/rax,r1/rbx,r2/rcx,r3/rdx,r4/rsi,r5/rdi,r6/rbp,r7/rsp
                    // r0/rax - buf address
                    // r1/rbx - end of buffer
                    // r2/rcx - iter_per_iter
                    // r3/rdx - write head
                    "movq    %0, %%rax \n\t"
                    "movq    %1, %%rbx \n\t"
                    "addq    %%rax, %%rbx \n\t"
                    "movq    %2, %%rcx \n"

                    "pushq   %%r15 \n\t"
                    "pushq   %%r14 \n\t"
                    "pushq   %%r13 \n\t"
                    "pushq   %%r12 \n\t"
                    "pushq   %%r11 \n\t"
                    "pushq   %%r10 \n\t"
                    "pushq   %%r9 \n\t"
                    "pushq   %%r8 \n\t"
                    //        "pushq   %%r7 \n\t"
                    //        "pushq   %%r6 \n\t"
                    //        "pushq   %%r5 \n\t"
                    //        "pushq   %%r4 \n\t"
                    //        "pushq   %%rdx \n\t"
                    //        "pushq   %%rcx \n\t"
                    //        "pushq   %%rbx \n\t"
                    //        "pushq   %%rax \n\t"

                    // load random data to register r8 ~ r15
                    "movabsq $6510615555426900480, %%r8 \n\t"
                    "movabsq $6510615555426900481, %%r9 \n\t"
                    "movabsq $6510615555426900482, %%r10 \n\t"
                    "movabsq $6510615555426900483, %%r11 \n\t"
                    "movabsq $6510615555426900484, %%r12 \n\t"
                    "movabsq $6510615555426900485, %%r13 \n\t"
                    "movabsq $6510615555426900486, %%r14 \n\t"
                    "movabsq $6510615555426900487, %%r15 \n\t"

                    "write_rewind%=: \n\t"
                    // write
                    "subq    $1, %%rcx \n\t"
                    "js      done_write%= \n\t"
                    "movq    %%rax, %%rdx \n"
                    "write_next1k%=: \n\t"
#if HAS_AVX512
                    // start writing
                    "vmovdqu8 %%zmm0,   0(%%rdx) \n\t"
                    "vmovdqu8 %%zmm1,  64(%%rdx) \n\t"
                    "vmovdqu8 %%zmm2, 128(%%rdx) \n\t"
                    "vmovdqu8 %%zmm3, 192(%%rdx) \n\t"
                    "vmovdqu8 %%zmm4, 256(%%rdx) \n\t"
                    "vmovdqu8 %%zmm5, 320(%%rdx) \n\t"
                    "vmovdqu8 %%zmm6, 384(%%rdx) \n\t"
                    "vmovdqu8 %%zmm7, 448(%%rdx) \n\t"
                    "vmovdqu8 %%zmm8, 512(%%rdx) \n\t"
                    "vmovdqu8 %%zmm9, 576(%%rdx) \n\t"
                    "vmovdqu8 %%zmm10, 640(%%rdx) \n\t"
                    "vmovdqu8 %%zmm11, 704(%%rdx) \n\t"
                    "vmovdqu8 %%zmm12, 768(%%rdx) \n\t"
                    "vmovdqu8 %%zmm13, 832(%%rdx) \n\t"
                    "vmovdqu8 %%zmm14, 896(%%rdx) \n\t"
                    "vmovdqu8 %%zmm15, 960(%%rdx) \n\t"
                    // end writing
#elif HAS_AVX2 || HAS_AVX
                    // using AVX/AVX2 instruction
                    // There are 16 256bit registers: YMM0-YMM15
                    // start writing
                    "vmovdqu %%ymm0, 0(%%rdx) \n\t"
                    "vmovdqu %%ymm1, 32(%%rdx) \n\t"
                    "vmovdqu %%ymm2, 64(%%rdx) \n\t"
                    "vmovdqu %%ymm3, 96(%%rdx) \n\t"
                    "vmovdqu %%ymm4, 128(%%rdx) \n\t"
                    "vmovdqu %%ymm5, 160(%%rdx) \n\t"
                    "vmovdqu %%ymm6, 192(%%rdx) \n\t"
                    "vmovdqu %%ymm7, 224(%%rdx) \n\t"
                    "vmovdqu %%ymm8, 256(%%rdx) \n\t"
                    "vmovdqu %%ymm9, 288(%%rdx) \n\t"
                    "vmovdqu %%ymm10, 320(%%rdx) \n\t"
                    "vmovdqu %%ymm11, 352(%%rdx) \n\t"
                    "vmovdqu %%ymm12, 384(%%rdx) \n\t"
                    "vmovdqu %%ymm13, 416(%%rdx) \n\t"
                    "vmovdqu %%ymm14, 448(%%rdx) \n\t"
                    "vmovdqu %%ymm15, 480(%%rdx) \n\t"
                    "vmovdqu %%ymm0, 512(%%rdx) \n\t"
                    "vmovdqu %%ymm1, 544(%%rdx) \n\t"
                    "vmovdqu %%ymm2, 576(%%rdx) \n\t"
                    "vmovdqu %%ymm3, 608(%%rdx) \n\t"
                    "vmovdqu %%ymm4, 640(%%rdx) \n\t"
                    "vmovdqu %%ymm5, 672(%%rdx) \n\t"
                    "vmovdqu %%ymm6, 704(%%rdx) \n\t"
                    "vmovdqu %%ymm7, 736(%%rdx) \n\t"
                    "vmovdqu %%ymm8, 768(%%rdx) \n\t"
                    "vmovdqu %%ymm9, 800(%%rdx) \n\t"
                    "vmovdqu %%ymm10, 832(%%rdx) \n\t"
                    "vmovdqu %%ymm11, 864(%%rdx) \n\t"
                    "vmovdqu %%ymm12, 896(%%rdx) \n\t"
                    "vmovdqu %%ymm13, 928(%%rdx) \n\t"
                    "vmovdqu %%ymm14, 960(%%rdx) \n\t"
                    "vmovdqu %%ymm15, 992(%%rdx) \n\t"
                    // end writing
#else
                    // start writing
                    "movq    %%r8, 0(%%rdx) \n\t"
                    "movq    %%r9, 8(%%rdx) \n\t"
                    "movq    %%r10, 16(%%rdx) \n\t"
                    "movq    %%r11, 24(%%rdx) \n\t"
                    "movq    %%r12, 32(%%rdx) \n\t"
                    "movq    %%r13, 40(%%rdx) \n\t"
                    "movq    %%r14, 48(%%rdx) \n\t"
                    "movq    %%r15, 56(%%rdx) \n\t"
                    "movq    %%r8, 64(%%rdx) \n\t"
                    "movq    %%r9, 72(%%rdx) \n\t"
                    "movq    %%r10, 80(%%rdx) \n\t"
                    "movq    %%r11, 88(%%rdx) \n\t"
                    "movq    %%r12, 96(%%rdx) \n\t"
                    "movq    %%r13, 104(%%rdx) \n\t"
                    "movq    %%r14, 112(%%rdx) \n\t"
                    "movq    %%r15, 120(%%rdx) \n\t"
                    "movq    %%r8, 128(%%rdx) \n\t"
                    "movq    %%r9, 136(%%rdx) \n\t"
                    "movq    %%r10, 144(%%rdx) \n\t"
                    "movq    %%r11, 152(%%rdx) \n\t"
                    "movq    %%r12, 160(%%rdx) \n\t"
                    "movq    %%r13, 168(%%rdx) \n\t"
                    "movq    %%r14, 176(%%rdx) \n\t"
                    "movq    %%r15, 184(%%rdx) \n\t"
                    "movq    %%r8, 192(%%rdx) \n\t"
                    "movq    %%r9, 200(%%rdx) \n\t"
                    "movq    %%r10, 208(%%rdx) \n\t"
                    "movq    %%r11, 216(%%rdx) \n\t"
                    "movq    %%r12, 224(%%rdx) \n\t"
                    "movq    %%r13, 232(%%rdx) \n\t"
                    "movq    %%r14, 240(%%rdx) \n\t"
                    "movq    %%r15, 248(%%rdx) \n\t"
                    "movq    %%r8, 256(%%rdx) \n\t"
                    "movq    %%r9, 264(%%rdx) \n\t"
                    "movq    %%r10, 272(%%rdx) \n\t"
                    "movq    %%r11, 280(%%rdx) \n\t"
                    "movq    %%r12, 288(%%rdx) \n\t"
                    "movq    %%r13, 296(%%rdx) \n\t"
                    "movq    %%r14, 304(%%rdx) \n\t"
                    "movq    %%r15, 312(%%rdx) \n\t"
                    "movq    %%r8, 320(%%rdx) \n\t"
                    "movq    %%r9, 328(%%rdx) \n\t"
                    "movq    %%r10, 336(%%rdx) \n\t"
                    "movq    %%r11, 344(%%rdx) \n\t"
                    "movq    %%r12, 352(%%rdx) \n\t"
                    "movq    %%r13, 360(%%rdx) \n\t"
                    "movq    %%r14, 368(%%rdx) \n\t"
                    "movq    %%r15, 376(%%rdx) \n\t"
                    "movq    %%r8, 384(%%rdx) \n\t"
                    "movq    %%r9, 392(%%rdx) \n\t"
                    "movq    %%r10, 400(%%rdx) \n\t"
                    "movq    %%r11, 408(%%rdx) \n\t"
                    "movq    %%r12, 416(%%rdx) \n\t"
                    "movq    %%r13, 424(%%rdx) \n\t"
                    "movq    %%r14, 432(%%rdx) \n\t"
                    "movq    %%r15, 440(%%rdx) \n\t"
                    "movq    %%r8, 448(%%rdx) \n\t"
                    "movq    %%r9, 456(%%rdx) \n\t"
                    "movq    %%r10, 464(%%rdx) \n\t"
                    "movq    %%r11, 472(%%rdx) \n\t"
                    "movq    %%r12, 480(%%rdx) \n\t"
                    "movq    %%r13, 488(%%rdx) \n\t"
                    "movq    %%r14, 496(%%rdx) \n\t"
                    "movq    %%r15, 504(%%rdx) \n\t"
                    "movq    %%r8, 512(%%rdx) \n\t"
                    "movq    %%r9, 520(%%rdx) \n\t"
                    "movq    %%r10, 528(%%rdx) \n\t"
                    "movq    %%r11, 536(%%rdx) \n\t"
                    "movq    %%r12, 544(%%rdx) \n\t"
                    "movq    %%r13, 552(%%rdx) \n\t"
                    "movq    %%r14, 560(%%rdx) \n\t"
                    "movq    %%r15, 568(%%rdx) \n\t"
                    "movq    %%r8, 576(%%rdx) \n\t"
                    "movq    %%r9, 584(%%rdx) \n\t"
                    "movq    %%r10, 592(%%rdx) \n\t"
                    "movq    %%r11, 600(%%rdx) \n\t"
                    "movq    %%r12, 608(%%rdx) \n\t"
                    "movq    %%r13, 616(%%rdx) \n\t"
                    "movq    %%r14, 624(%%rdx) \n\t"
                    "movq    %%r15, 632(%%rdx) \n\t"
                    "movq    %%r8, 640(%%rdx) \n\t"
                    "movq    %%r9, 648(%%rdx) \n\t"
                    "movq    %%r10, 656(%%rdx) \n\t"
                    "movq    %%r11, 664(%%rdx) \n\t"
                    "movq    %%r12, 672(%%rdx) \n\t"
                    "movq    %%r13, 680(%%rdx) \n\t"
                    "movq    %%r14, 688(%%rdx) \n\t"
                    "movq    %%r15, 696(%%rdx) \n\t"
                    "movq    %%r8, 704(%%rdx) \n\t"
                    "movq    %%r9, 712(%%rdx) \n\t"
                    "movq    %%r10, 720(%%rdx) \n\t"
                    "movq    %%r11, 728(%%rdx) \n\t"
                    "movq    %%r12, 736(%%rdx) \n\t"
                    "movq    %%r13, 744(%%rdx) \n\t"
                    "movq    %%r14, 752(%%rdx) \n\t"
                    "movq    %%r15, 760(%%rdx) \n\t"
                    "movq    %%r8, 768(%%rdx) \n\t"
                    "movq    %%r9, 776(%%rdx) \n\t"
                    "movq    %%r10, 784(%%rdx) \n\t"
                    "movq    %%r11, 792(%%rdx) \n\t"
                    "movq    %%r12, 800(%%rdx) \n\t"
                    "movq    %%r13, 808(%%rdx) \n\t"
                    "movq    %%r14, 816(%%rdx) \n\t"
                    "movq    %%r15, 824(%%rdx) \n\t"
                    "movq    %%r8, 832(%%rdx) \n\t"
                    "movq    %%r9, 840(%%rdx) \n\t"
                    "movq    %%r10, 848(%%rdx) \n\t"
                    "movq    %%r11, 856(%%rdx) \n\t"
                    "movq    %%r12, 864(%%rdx) \n\t"
                    "movq    %%r13, 872(%%rdx) \n\t"
                    "movq    %%r14, 880(%%rdx) \n\t"
                    "movq    %%r15, 888(%%rdx) \n\t"
                    "movq    %%r8, 896(%%rdx) \n\t"
                    "movq    %%r9, 904(%%rdx) \n\t"
                    "movq    %%r10, 912(%%rdx) \n\t"
                    "movq    %%r11, 920(%%rdx) \n\t"
                    "movq    %%r12, 928(%%rdx) \n\t"
                    "movq    %%r13, 936(%%rdx) \n\t"
                    "movq    %%r14, 944(%%rdx) \n\t"
                    "movq    %%r15, 952(%%rdx) \n\t"
                    "movq    %%r8, 960(%%rdx) \n\t"
                    "movq    %%r9, 968(%%rdx) \n\t"
                    "movq    %%r10, 976(%%rdx) \n\t"
                    "movq    %%r11, 984(%%rdx) \n\t"
                    "movq    %%r12, 992(%%rdx) \n\t"
                    "movq    %%r13, 1000(%%rdx) \n\t"
                    "movq    %%r14, 1008(%%rdx) \n\t"
                    "movq    %%r15, 1016(%%rdx) \n\t"
                    // end of writing
#endif
                    "addq    $1024, %%rdx \n\t"
                    "cmpq    %%rdx, %%rbx \n\t"
                    "je      write_rewind%= \n\t"
                    "jmp     write_next1k%= \n"

                    "done_write%=: \n\t"

                    //        "popq    %%rax \n\t"
                    //        "popq    %%rbx \n\t"
                    //        "popq    %%rcx \n\t"
                    //        "popq    %%rdx \n\t"
                    //        "popq    %%r4 \n\t"
                    //        "popq    %%r5 \n\t"
                    //        "popq    %%r6 \n\t"
                    //        "popq    %%r7 \n\t"
                    "popq    %%r8 \n\t"
                    "popq    %%r9 \n\t"
                    "popq    %%r10 \n\t"
                    "popq    %%r11 \n\t"
                    "popq    %%r12 \n\t"
                    "popq    %%r13 \n\t"
                    "popq    %%r14 \n\t"
                    "popq    %%r15 \n\t"
                    :  // no output
                    : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
                    : "rax", "rbx", "rcx", "rdx");
#elif defined(__aarch64__)
	    // -- write
            __asm__ volatile(
                    // x0-x7 are for the arguments:
                    // x0 - buf address
                    // x1 - buffer size
                    // x2 - iter_per_iter
                    // x3 - write head
                    // x8 is for return value
                    // x9-x15 are corruptible
                    // we are using x5 to x12
                    "ldr    x0, %0 \n\t"
                    "ldr    x1, %1 \n\t"
                    "add    x1, x0, x1 \n\t" // x1 = end of buffer
                    "ldr    x2, %2 \n\t"
                    "stp    x4, x5, [sp, -16]! \n\t"
                    "stp    x6, x7, [sp, -16]! \n\t"
                    "stp    x8, x9, [sp, -16]! \n\t"
                    "stp    x10, x11, [sp, -16]! \n\t"

                    "write_rewind: \n\t"
                    "mov    x3, x0 \n\t"
                    "write_next1k: \n\t"
                    // write
                    "str    x4, [x3, 0] \n\t"
                    "str    x5, [x3, 8] \n\t"
                    "str    x6, [x3, 16] \n\t"
                    "str    x7, [x3, 24] \n\t"
                    "str    x8, [x3, 32] \n\t"
                    "str    x9, [x3, 40] \n\t"
                    "str    x10, [x3, 48] \n\t"
                    "str    x11, [x3, 56] \n\t"
                    "str    x4, [x3, 64] \n\t"
                    "str    x5, [x3, 72] \n\t"
                    "str    x6, [x3, 80] \n\t"
                    "str    x7, [x3, 88] \n\t"
                    "str    x8, [x3, 96] \n\t"
                    "str    x9, [x3, 104] \n\t"
                    "str    x10, [x3, 112] \n\t"
                    "str    x11, [x3, 120] \n\t"
                    "str    x4, [x3, 128] \n\t"
                    "str    x5, [x3, 136] \n\t"
                    "str    x6, [x3, 144] \n\t"
                    "str    x7, [x3, 152] \n\t"
                    "str    x8, [x3, 160] \n\t"
                    "str    x9, [x3, 168] \n\t"
                    "str    x10, [x3, 176] \n\t"
                    "str    x11, [x3, 184] \n\t"
                    "str    x4, [x3, 192] \n\t"
                    "str    x5, [x3, 200] \n\t"
                    "str    x6, [x3, 208] \n\t"
                    "str    x7, [x3, 216] \n\t"
                    "str    x8, [x3, 224] \n\t"
                    "str    x9, [x3, 232] \n\t"
                    "str    x10, [x3, 240] \n\t"
                    "str    x11, [x3, 248] \n\t"
                    "str    x4, [x3, 256] \n\t"
                    "str    x5, [x3, 264] \n\t"
                    "str    x6, [x3, 272] \n\t"
                    "str    x7, [x3, 280] \n\t"
                    "str    x8, [x3, 288] \n\t"
                    "str    x9, [x3, 296] \n\t"
                    "str    x10, [x3, 304] \n\t"
                    "str    x11, [x3, 312] \n\t"
                    "str    x4, [x3, 320] \n\t"
                    "str    x5, [x3, 328] \n\t"
                    "str    x6, [x3, 336] \n\t"
                    "str    x7, [x3, 344] \n\t"
                    "str    x8, [x3, 352] \n\t"
                    "str    x9, [x3, 360] \n\t"
                    "str    x10, [x3, 368] \n\t"
                    "str    x11, [x3, 376] \n\t"
                    "str    x4, [x3, 384] \n\t"
                    "str    x5, [x3, 392] \n\t"
                    "str    x6, [x3, 400] \n\t"
                    "str    x7, [x3, 408] \n\t"
                    "str    x8, [x3, 416] \n\t"
                    "str    x9, [x3, 424] \n\t"
                    "str    x10, [x3, 432] \n\t"
                    "str    x11, [x3, 440] \n\t"
                    "str    x4, [x3, 448] \n\t"
                    "str    x5, [x3, 456] \n\t"
                    "str    x6, [x3, 464] \n\t"
                    "str    x7, [x3, 472] \n\t"
                    "str    x8, [x3, 480] \n\t"
                    "str    x9, [x3, 488] \n\t"
                    "str    x10, [x3, 496] \n\t"
                    "str    x11, [x3, 504] \n\t"
                    "str    x4, [x3, 512] \n\t"
                    "str    x5, [x3, 520] \n\t"
                    "str    x6, [x3, 528] \n\t"
                    "str    x7, [x3, 536] \n\t"
                    "str    x8, [x3, 544] \n\t"
                    "str    x8, [x3, 552] \n\t"
                    "str    x10, [x3, 560] \n\t"
                    "str    x11, [x3, 568] \n\t"
                    "str    x4, [x3, 576] \n\t"
                    "str    x5, [x3, 584] \n\t"
                    "str    x6, [x3, 592] \n\t"
                    "str    x7, [x3, 600] \n\t"
                    "str    x8, [x3, 608] \n\t"
                    "str    x9, [x3, 616] \n\t"
                    "str    x10, [x3, 624] \n\t"
                    "str    x11, [x3, 632] \n\t"
                    "str    x4, [x3, 640] \n\t"
                    "str    x5, [x3, 648] \n\t"
                    "str    x6, [x3, 656] \n\t"
                    "str    x7, [x3, 664] \n\t"
                    "str    x8, [x3, 672] \n\t"
                    "str    x9, [x3, 680] \n\t"
                    "str    x10, [x3, 688] \n\t"
                    "str    x11, [x3, 696] \n\t"
                    "str    x4, [x3, 704] \n\t"
                    "str    x5, [x3, 712] \n\t"
                    "str    x6, [x3, 720] \n\t"
                    "str    x7, [x3, 728] \n\t"
                    "str    x8, [x3, 736] \n\t"
                    "str    x9, [x3, 744] \n\t"
                    "str    x10, [x3, 752] \n\t"
                    "str    x11, [x3, 760] \n\t"
                    "str    x4, [x3, 768] \n\t"
                    "str    x5, [x3, 776] \n\t"
                    "str    x6, [x3, 784] \n\t"
                    "str    x7, [x3, 792] \n\t"
                    "str    x8, [x3, 800] \n\t"
                    "str    x9, [x3, 808] \n\t"
                    "str    x10, [x3, 816] \n\t"
                    "str    x11, [x3, 824] \n\t"
                    "str    x4, [x3, 832] \n\t"
                    "str    x5, [x3, 840] \n\t"
                    "str    x6, [x3, 848] \n\t"
                    "str    x7, [x3, 856] \n\t"
                    "str    x8, [x3, 864] \n\t"
                    "str    x9, [x3, 872] \n\t"
                    "str    x10, [x3, 880] \n\t"
                    "str    x11, [x3, 888] \n\t"
                    "str    x4, [x3, 896] \n\t"
                    "str    x5, [x3, 904] \n\t"
                    "str    x6, [x3, 912] \n\t"
                    "str    x7, [x3, 920] \n\t"
                    "str    x8, [x3, 928] \n\t"
                    "str    x9, [x3, 936] \n\t"
                    "str    x10, [x3, 944] \n\t"
                    "str    x11, [x3, 952] \n\t"
                    "str    x4, [x3, 960] \n\t"
                    "str    x5, [x3, 968] \n\t"
                    "str    x6, [x3, 976] \n\t"
                    "str    x7, [x3, 984] \n\t"
                    "str    x8, [x3, 992] \n\t"
                    "str    x9, [x3, 1000] \n\t"
                    "str    x10, [x3, 1008] \n\t"
                    "str    x11, [x3, 1016] \n\t"
                    // next round
                    "add    x3, x3, 1024 \n\t"
                    "cmp    x3, x1 \n\t"
                    "bne    write_next1k \n\t"
                    "subs   x2, x2, 1 \n\t"
                    // done
                    "beq    done_write \n\t"
                    "b      write_rewind \n\t"
                    "done_write: \n\t"
                    "ldp    x10, x11, [sp], 16 \n\t"
                    "ldp    x8, x9, [sp], 16 \n\t"
                    "ldp    x6, x7, [sp], 16 \n\t"
                    "ldp    x4, x5, [sp], 16 \n\t"
                    : // no output
                    : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
                    : "x0", "x1", "x2", "x3");
#endif
        } else {
#if defined(__x86_64__)
            // -- read
            __asm__ volatile(
                    // r0/rax,r1/rbx,r2/rcx,r3/rdx,r4/rsi,r5/rdi,r6/rbp,r7/rsp
                    // r0/rax - buf address
                    // r1/rbx - end of buffer
                    // r2/rcx - iter_per_iter
                    // r3/rdx - read head
                    "movq    %0, %%rax \n\t"
                    "movq    %1, %%rbx \n\t"
                    "addq    %%rax, %%rbx \n\t"
                    "movq    %2, %%rcx \n"

                    "pushq   %%r15 \n\t"
                    "pushq   %%r14 \n\t"
                    "pushq   %%r13 \n\t"
                    "pushq   %%r12 \n\t"
                    "pushq   %%r11 \n\t"
                    "pushq   %%r10 \n\t"
                    "pushq   %%r9 \n\t"
                    "pushq   %%r8 \n\t"
                    //        "pushq   %%r7 \n\t"
                    //        "pushq   %%r6 \n\t"
                    //        "pushq   %%r5 \n\t"
                    //        "pushq   %%r4 \n\t"
                    //        "pushq   %%rdx \n\t"
                    //        "pushq   %%rcx \n\t"
                    //        "pushq   %%rbx \n\t"
                    //        "pushq   %%rax \n\t"

                    "read_rewind%=: \n\t"
                    // read
                    "subq    $1, %%rcx \n\t"
                    "js      done_read%= \n\t"
                    "movq    %%rax, %%rdx \n"
                    "read_next1k%=: \n\t"
#if HAS_AVX512
                    // start reading
                    "vmovdqu8   0(%%rdx), %%zmm0 \n\t"
                    "vmovdqu8  64(%%rdx), %%zmm1 \n\t"
                    "vmovdqu8 128(%%rdx), %%zmm2 \n\t"
                    "vmovdqu8 192(%%rdx), %%zmm3 \n\t"
                    "vmovdqu8 256(%%rdx), %%zmm4 \n\t"
                    "vmovdqu8 320(%%rdx), %%zmm5 \n\t"
                    "vmovdqu8 384(%%rdx), %%zmm6 \n\t"
                    "vmovdqu8 448(%%rdx), %%zmm7 \n\t"
                    "vmovdqu8 512(%%rdx), %%zmm8 \n\t"
                    "vmovdqu8 576(%%rdx), %%zmm9 \n\t"
                    "vmovdqu8 640(%%rdx), %%zmm10 \n\t"
                    "vmovdqu8 704(%%rdx), %%zmm11 \n\t"
                    "vmovdqu8 768(%%rdx), %%zmm12 \n\t"
                    "vmovdqu8 832(%%rdx), %%zmm13 \n\t"
                    "vmovdqu8 896(%%rdx), %%zmm14 \n\t"
                    "vmovdqu8 960(%%rdx), %%zmm15 \n\t"
                    // end reading
#elif HAS_AVX2 || HAS_AVX
                    // using AVX/AVX2 instruction
                    // There are 16 256bit registers: YMM0-YMM15
                    // start reading
                    "vmovdqu   0(%%rdx), %%ymm0 \n\t"
                    "vmovdqu  32(%%rdx), %%ymm1 \n\t"
                    "vmovdqu  64(%%rdx), %%ymm2 \n\t"
                    "vmovdqu  96(%%rdx), %%ymm3 \n\t"
                    "vmovdqu 128(%%rdx), %%ymm4 \n\t"
                    "vmovdqu 160(%%rdx), %%ymm5 \n\t"
                    "vmovdqu 192(%%rdx), %%ymm6 \n\t"
                    "vmovdqu 224(%%rdx), %%ymm7 \n\t"
                    "vmovdqu 256(%%rdx), %%ymm8 \n\t"
                    "vmovdqu 288(%%rdx), %%ymm9 \n\t"
                    "vmovdqu 320(%%rdx), %%ymm10 \n\t"
                    "vmovdqu 352(%%rdx), %%ymm11 \n\t"
                    "vmovdqu 384(%%rdx), %%ymm12 \n\t"
                    "vmovdqu 416(%%rdx), %%ymm13 \n\t"
                    "vmovdqu 448(%%rdx), %%ymm14 \n\t"
                    "vmovdqu 480(%%rdx), %%ymm15 \n\t"
                    "vmovdqu 512(%%rdx), %%ymm0 \n\t"
                    "vmovdqu 544(%%rdx), %%ymm1 \n\t"
                    "vmovdqu 576(%%rdx), %%ymm2 \n\t"
                    "vmovdqu 608(%%rdx), %%ymm3 \n\t"
                    "vmovdqu 640(%%rdx), %%ymm4 \n\t"
                    "vmovdqu 672(%%rdx), %%ymm5 \n\t"
                    "vmovdqu 704(%%rdx), %%ymm6 \n\t"
                    "vmovdqu 736(%%rdx), %%ymm7 \n\t"
                    "vmovdqu 768(%%rdx), %%ymm8 \n\t"
                    "vmovdqu 800(%%rdx), %%ymm9 \n\t"
                    "vmovdqu 832(%%rdx), %%ymm10 \n\t"
                    "vmovdqu 864(%%rdx), %%ymm11 \n\t"
                    "vmovdqu 896(%%rdx), %%ymm12 \n\t"
                    "vmovdqu 928(%%rdx), %%ymm13 \n\t"
                    "vmovdqu 960(%%rdx), %%ymm14 \n\t"
                    "vmovdqu 992(%%rdx), %%ymm15 \n\t"
                    // end reading
#else
                    // start reading
                    "movq    0(%%rdx), %%r8 \n\t"
                    "movq    8(%%rdx), %%r9 \n\t"
                    "movq    16(%%rdx), %%r10 \n\t"
                    "movq    24(%%rdx), %%r11 \n\t"
                    "movq    32(%%rdx), %%r12 \n\t"
                    "movq    40(%%rdx), %%r13 \n\t"
                    "movq    48(%%rdx), %%r14 \n\t"
                    "movq    56(%%rdx), %%r15 \n\t"
                    "movq    64(%%rdx), %%r8 \n\t"
                    "movq    72(%%rdx), %%r9 \n\t"
                    "movq    80(%%rdx), %%r10 \n\t"
                    "movq    88(%%rdx), %%r11 \n\t"
                    "movq    96(%%rdx), %%r12 \n\t"
                    "movq    104(%%rdx), %%r13 \n\t"
                    "movq    112(%%rdx), %%r14 \n\t"
                    "movq    120(%%rdx), %%r15 \n\t"
                    "movq    128(%%rdx), %%r8 \n\t"
                    "movq    136(%%rdx), %%r9 \n\t"
                    "movq    144(%%rdx), %%r10 \n\t"
                    "movq    152(%%rdx), %%r11 \n\t"
                    "movq    160(%%rdx), %%r12 \n\t"
                    "movq    168(%%rdx), %%r13 \n\t"
                    "movq    176(%%rdx), %%r14 \n\t"
                    "movq    184(%%rdx), %%r15 \n\t"
                    "movq    192(%%rdx), %%r8 \n\t"
                    "movq    200(%%rdx), %%r9 \n\t"
                    "movq    208(%%rdx), %%r10 \n\t"
                    "movq    216(%%rdx), %%r11 \n\t"
                    "movq    224(%%rdx), %%r12 \n\t"
                    "movq    232(%%rdx), %%r13 \n\t"
                    "movq    240(%%rdx), %%r14 \n\t"
                    "movq    248(%%rdx), %%r15 \n\t"
                    "movq    256(%%rdx), %%r8 \n\t"
                    "movq    264(%%rdx), %%r9 \n\t"
                    "movq    272(%%rdx), %%r10 \n\t"
                    "movq    280(%%rdx), %%r11 \n\t"
                    "movq    288(%%rdx), %%r12 \n\t"
                    "movq    296(%%rdx), %%r13 \n\t"
                    "movq    304(%%rdx), %%r14 \n\t"
                    "movq    312(%%rdx), %%r15 \n\t"
                    "movq    320(%%rdx), %%r8 \n\t"
                    "movq    328(%%rdx), %%r9 \n\t"
                    "movq    336(%%rdx), %%r10 \n\t"
                    "movq    344(%%rdx), %%r11 \n\t"
                    "movq    352(%%rdx), %%r12 \n\t"
                    "movq    360(%%rdx), %%r13 \n\t"
                    "movq    368(%%rdx), %%r14 \n\t"
                    "movq    376(%%rdx), %%r15 \n\t"
                    "movq    384(%%rdx), %%r8 \n\t"
                    "movq    392(%%rdx), %%r9 \n\t"
                    "movq    400(%%rdx), %%r10 \n\t"
                    "movq    408(%%rdx), %%r11 \n\t"
                    "movq    416(%%rdx), %%r12 \n\t"
                    "movq    424(%%rdx), %%r13 \n\t"
                    "movq    432(%%rdx), %%r14 \n\t"
                    "movq    440(%%rdx), %%r15 \n\t"
                    "movq    448(%%rdx), %%r8 \n\t"
                    "movq    456(%%rdx), %%r9 \n\t"
                    "movq    464(%%rdx), %%r10 \n\t"
                    "movq    472(%%rdx), %%r11 \n\t"
                    "movq    480(%%rdx), %%r12 \n\t"
                    "movq    488(%%rdx), %%r13 \n\t"
                    "movq    496(%%rdx), %%r14 \n\t"
                    "movq    504(%%rdx), %%r15 \n\t"
                    "movq    512(%%rdx), %%r8 \n\t"
                    "movq    520(%%rdx), %%r9 \n\t"
                    "movq    528(%%rdx), %%r10 \n\t"
                    "movq    536(%%rdx), %%r11 \n\t"
                    "movq    544(%%rdx), %%r12 \n\t"
                    "movq    552(%%rdx), %%r13 \n\t"
                    "movq    560(%%rdx), %%r14 \n\t"
                    "movq    568(%%rdx), %%r15 \n\t"
                    "movq    576(%%rdx), %%r8 \n\t"
                    "movq    584(%%rdx), %%r9 \n\t"
                    "movq    592(%%rdx), %%r10 \n\t"
                    "movq    600(%%rdx), %%r11 \n\t"
                    "movq    608(%%rdx), %%r12 \n\t"
                    "movq    616(%%rdx), %%r13 \n\t"
                    "movq    624(%%rdx), %%r14 \n\t"
                    "movq    632(%%rdx), %%r15 \n\t"
                    "movq    640(%%rdx), %%r8 \n\t"
                    "movq    648(%%rdx), %%r9 \n\t"
                    "movq    656(%%rdx), %%r10 \n\t"
                    "movq    664(%%rdx), %%r11 \n\t"
                    "movq    672(%%rdx), %%r12 \n\t"
                    "movq    680(%%rdx), %%r13 \n\t"
                    "movq    688(%%rdx), %%r14 \n\t"
                    "movq    696(%%rdx), %%r15 \n\t"
                    "movq    704(%%rdx), %%r8 \n\t"
                    "movq    712(%%rdx), %%r9 \n\t"
                    "movq    720(%%rdx), %%r10 \n\t"
                    "movq    728(%%rdx), %%r11 \n\t"
                    "movq    736(%%rdx), %%r12 \n\t"
                    "movq    744(%%rdx), %%r13 \n\t"
                    "movq    752(%%rdx), %%r14 \n\t"
                    "movq    760(%%rdx), %%r15 \n\t"
                    "movq    768(%%rdx), %%r8 \n\t"
                    "movq    776(%%rdx), %%r9 \n\t"
                    "movq    784(%%rdx), %%r10 \n\t"
                    "movq    792(%%rdx), %%r11 \n\t"
                    "movq    800(%%rdx), %%r12 \n\t"
                    "movq    808(%%rdx), %%r13 \n\t"
                    "movq    816(%%rdx), %%r14 \n\t"
                    "movq    824(%%rdx), %%r15 \n\t"
                    "movq    832(%%rdx), %%r8 \n\t"
                    "movq    840(%%rdx), %%r9 \n\t"
                    "movq    848(%%rdx), %%r10 \n\t"
                    "movq    856(%%rdx), %%r11 \n\t"
                    "movq    864(%%rdx), %%r12 \n\t"
                    "movq    872(%%rdx), %%r13 \n\t"
                    "movq    880(%%rdx), %%r14 \n\t"
                    "movq    888(%%rdx), %%r15 \n\t"
                    "movq    896(%%rdx), %%r8 \n\t"
                    "movq    904(%%rdx), %%r9 \n\t"
                    "movq    912(%%rdx), %%r10 \n\t"
                    "movq    920(%%rdx), %%r11 \n\t"
                    "movq    928(%%rdx), %%r12 \n\t"
                    "movq    936(%%rdx), %%r13 \n\t"
                    "movq    944(%%rdx), %%r14 \n\t"
                    "movq    952(%%rdx), %%r15 \n\t"
                    "movq    960(%%rdx), %%r8 \n\t"
                    "movq    968(%%rdx), %%r9 \n\t"
                    "movq    976(%%rdx), %%r10 \n\t"
                    "movq    984(%%rdx), %%r11 \n\t"
                    "movq    992(%%rdx), %%r12 \n\t"
                    "movq    1000(%%rdx), %%r13 \n\t"
                    "movq    1008(%%rdx), %%r14 \n\t"
                    "movq    1016(%%rdx), %%r15 \n\t"
                    // end of reading
#endif
                    "addq    $1024, %%rdx \n\t"
                    "cmpq    %%rdx, %%rbx \n\t"
                    "je      read_rewind%= \n\t"
                    "jmp     read_next1k%= \n"

                    "done_read%=: \n\t"

                    //        "popq    %%rax \n\t"
                    //        "popq    %%rbx \n\t"
                    //        "popq    %%rcx \n\t"
                    //        "popq    %%rdx \n\t"
                    //        "popq    %%r4 \n\t"
                    //        "popq    %%r5 \n\t"
                    //        "popq    %%r6 \n\t"
                    //        "popq    %%r7 \n\t"
                    "popq    %%r8 \n\t"
                    "popq    %%r9 \n\t"
                    "popq    %%r10 \n\t"
                    "popq    %%r11 \n\t"
                    "popq    %%r12 \n\t"
                    "popq    %%r13 \n\t"
                    "popq    %%r14 \n\t"
                    "popq    %%r15 \n\t"
                    :  // no output
                    : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
                    : "rax", "rbx", "rcx", "rdx");
#elif defined(__aarch64__)
	    // -- read
            __asm__ volatile(
                    // x0-x7 are for the arguments:
                    // x0 - buf address
                    // x1 - buffer size
                    // x2 - iter_per_iter
                    // x3 - read head
                    // x8 is for return value
                    // x9-x15 are corruptible
                    // we are using x5 to x12
                    "ldr    x0, %0 \n\t"
                    "ldr    x1, %1 \n\t"
                    "add    x1, x0, x1 \n\t" // x1 = end of buffer
                    "ldr    x2, %2 \n\t"
                    "stp    x4, x5, [sp, -16]! \n\t"
                    "stp    x6, x7, [sp, -16]! \n\t"
                    "stp    x8, x9, [sp, -16]! \n\t"
                    "stp    x10, x11, [sp, -16]! \n\t"

                    "read_rewind: \n\t"
                    "mov    x3, x0 \n\t"
                    "read_next1k: \n\t"
                    // read
                    "ldr    x4, [x3, 0] \n\t"
                    "ldr    x5, [x3, 8] \n\t"
                    "ldr    x6, [x3, 16] \n\t"
                    "ldr    x7, [x3, 24] \n\t"
                    "ldr    x8, [x3, 32] \n\t"
                    "ldr    x9, [x3, 40] \n\t"
                    "ldr    x10, [x3, 48] \n\t"
                    "ldr    x11, [x3, 56] \n\t"
                    "ldr    x4, [x3, 64] \n\t"
                    "ldr    x5, [x3, 72] \n\t"
                    "ldr    x6, [x3, 80] \n\t"
                    "ldr    x7, [x3, 88] \n\t"
                    "ldr    x8, [x3, 96] \n\t"
                    "ldr    x9, [x3, 104] \n\t"
                    "ldr    x10, [x3, 112] \n\t"
                    "ldr    x11, [x3, 120] \n\t"
                    "ldr    x4, [x3, 128] \n\t"
                    "ldr    x5, [x3, 136] \n\t"
                    "ldr    x6, [x3, 144] \n\t"
                    "ldr    x7, [x3, 152] \n\t"
                    "ldr    x8, [x3, 160] \n\t"
                    "ldr    x9, [x3, 168] \n\t"
                    "ldr    x10, [x3, 176] \n\t"
                    "ldr    x11, [x3, 184] \n\t"
                    "ldr    x4, [x3, 192] \n\t"
                    "ldr    x5, [x3, 200] \n\t"
                    "ldr    x6, [x3, 208] \n\t"
                    "ldr    x7, [x3, 216] \n\t"
                    "ldr    x8, [x3, 224] \n\t"
                    "ldr    x9, [x3, 232] \n\t"
                    "ldr    x10, [x3, 240] \n\t"
                    "ldr    x11, [x3, 248] \n\t"
                    "ldr    x4, [x3, 256] \n\t"
                    "ldr    x5, [x3, 264] \n\t"
                    "ldr    x6, [x3, 272] \n\t"
                    "ldr    x7, [x3, 280] \n\t"
                    "ldr    x8, [x3, 288] \n\t"
                    "ldr    x9, [x3, 296] \n\t"
                    "ldr    x10, [x3, 304] \n\t"
                    "ldr    x11, [x3, 312] \n\t"
                    "ldr    x4, [x3, 320] \n\t"
                    "ldr    x5, [x3, 328] \n\t"
                    "ldr    x6, [x3, 336] \n\t"
                    "ldr    x7, [x3, 344] \n\t"
                    "ldr    x8, [x3, 352] \n\t"
                    "ldr    x9, [x3, 360] \n\t"
                    "ldr    x10, [x3, 368] \n\t"
                    "ldr    x11, [x3, 376] \n\t"
                    "ldr    x4, [x3, 384] \n\t"
                    "ldr    x5, [x3, 392] \n\t"
                    "ldr    x6, [x3, 400] \n\t"
                    "ldr    x7, [x3, 408] \n\t"
                    "ldr    x8, [x3, 416] \n\t"
                    "ldr    x9, [x3, 424] \n\t"
                    "ldr    x10, [x3, 432] \n\t"
                    "ldr    x11, [x3, 440] \n\t"
                    "ldr    x4, [x3, 448] \n\t"
                    "ldr    x5, [x3, 456] \n\t"
                    "ldr    x6, [x3, 464] \n\t"
                    "ldr    x7, [x3, 472] \n\t"
                    "ldr    x8, [x3, 480] \n\t"
                    "ldr    x9, [x3, 488] \n\t"
                    "ldr    x10, [x3, 496] \n\t"
                    "ldr    x11, [x3, 504] \n\t"
                    "ldr    x4, [x3, 512] \n\t"
                    "ldr    x5, [x3, 520] \n\t"
                    "ldr    x6, [x3, 528] \n\t"
                    "ldr    x7, [x3, 536] \n\t"
                    "ldr    x8, [x3, 544] \n\t"
                    "ldr    x8, [x3, 552] \n\t"
                    "ldr    x10, [x3, 560] \n\t"
                    "ldr    x11, [x3, 568] \n\t"
                    "ldr    x4, [x3, 576] \n\t"
                    "ldr    x5, [x3, 584] \n\t"
                    "ldr    x6, [x3, 592] \n\t"
                    "ldr    x7, [x3, 600] \n\t"
                    "ldr    x8, [x3, 608] \n\t"
                    "ldr    x9, [x3, 616] \n\t"
                    "ldr    x10, [x3, 624] \n\t"
                    "ldr    x11, [x3, 632] \n\t"
                    "ldr    x4, [x3, 640] \n\t"
                    "ldr    x5, [x3, 648] \n\t"
                    "ldr    x6, [x3, 656] \n\t"
                    "ldr    x7, [x3, 664] \n\t"
                    "ldr    x8, [x3, 672] \n\t"
                    "ldr    x9, [x3, 680] \n\t"
                    "ldr    x10, [x3, 688] \n\t"
                    "ldr    x11, [x3, 696] \n\t"
                    "ldr    x4, [x3, 704] \n\t"
                    "ldr    x5, [x3, 712] \n\t"
                    "ldr    x6, [x3, 720] \n\t"
                    "ldr    x7, [x3, 728] \n\t"
                    "ldr    x8, [x3, 736] \n\t"
                    "ldr    x9, [x3, 744] \n\t"
                    "ldr    x10, [x3, 752] \n\t"
                    "ldr    x11, [x3, 760] \n\t"
                    "ldr    x4, [x3, 768] \n\t"
                    "ldr    x5, [x3, 776] \n\t"
                    "ldr    x6, [x3, 784] \n\t"
                    "ldr    x7, [x3, 792] \n\t"
                    "ldr    x8, [x3, 800] \n\t"
                    "ldr    x9, [x3, 808] \n\t"
                    "ldr    x10, [x3, 816] \n\t"
                    "ldr    x11, [x3, 824] \n\t"
                    "ldr    x4, [x3, 832] \n\t"
                    "ldr    x5, [x3, 840] \n\t"
                    "ldr    x6, [x3, 848] \n\t"
                    "ldr    x7, [x3, 856] \n\t"
                    "ldr    x8, [x3, 864] \n\t"
                    "ldr    x9, [x3, 872] \n\t"
                    "ldr    x10, [x3, 880] \n\t"
                    "ldr    x11, [x3, 888] \n\t"
                    "ldr    x4, [x3, 896] \n\t"
                    "ldr    x5, [x3, 904] \n\t"
                    "ldr    x6, [x3, 912] \n\t"
                    "ldr    x7, [x3, 920] \n\t"
                    "ldr    x8, [x3, 928] \n\t"
                    "ldr    x9, [x3, 936] \n\t"
                    "ldr    x10, [x3, 944] \n\t"
                    "ldr    x11, [x3, 952] \n\t"
                    "ldr    x4, [x3, 960] \n\t"
                    "ldr    x5, [x3, 968] \n\t"
                    "ldr    x6, [x3, 976] \n\t"
                    "ldr    x7, [x3, 984] \n\t"
                    "ldr    x8, [x3, 992] \n\t"
                    "ldr    x9, [x3, 1000] \n\t"
                    "ldr    x10, [x3, 1008] \n\t"
                    "ldr    x11, [x3, 1016] \n\t"
                    // next round
                    "add    x3, x3, 1024 \n\t"
                    "cmp    x3, x1 \n\t"
                    "bne    read_next1k \n\t"
                    "subs   x2, x2, 1 \n\t"
                    // done
                    "beq    done_read \n\t"
                    "b      read_rewind \n\t"
                    "done_read: \n\t"
                    "ldp    x10, x11, [sp], 16 \n\t"
                    "ldp    x8, x9, [sp], 16 \n\t"
                    "ldp    x6, x7, [sp], 16 \n\t"
                    "ldp    x4, x5, [sp], 16 \n\t"
                    : // no output
                    : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
                    : "x0", "x1", "x2", "x3");
#endif
        }

        // STEP 6 - end the timer
        if (timing == CLOCK_GETTIME) {
            ret = clock_gettime(CLOCK_MONOTONIC, &clk_te);
            RETURN_ON_ERROR(ret, "clock_gettime");
        } else if (timing == RDTSC) {
            tsc_te = rdtsc();
        }

        lpcs.stop_perf_events();

        // STEP 7 - calculate throguhput
        if (timing == PERF_CPU_CYCLE) {
            results[iter] = (iter_per_iter)*buffer_size / static_cast<double>(lpcs.get().at("cpu_cycles(pf)"));
        } else if (timing == HW_CPU_CYCLE) {
            results[iter] = (iter_per_iter)*buffer_size / static_cast<double>(lpcs.get().at("cpu_cycles(hw)"));
        } else if (timing == RDTSC) {
            results[iter] = THROUGHPUT_BYTES_PER_CYCLE((iter_per_iter)*buffer_size, tsc_ts, tsc_te);
        } else if (timing == CLOCK_GETTIME) {
            results[iter] = THROUGHPUT_GiBPS((iter_per_iter)*buffer_size, clk_ts, clk_te);
        }

        // STEP 8 - collect counters
        if(counters.has_value()) {
            counters->emplace_back(lpcs.get());
        }
    }

    // STEP 8 - clean up
    if(buffer == nullptr) {
#if USE_HUGEPAGE
        munmap(buf, buffer_size);
#else
        free(buf);
#endif
    }

    return 0;
}
}//namespace cacheinspector
