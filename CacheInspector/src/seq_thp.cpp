#include <assert.h>
#include <errno.h>
#include <iostream>
#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <optional>
#include <unistd.h>

#if USE_HUGEPAGE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#define FILE_NAME "/mnt/huge/hugepagefile"
#define ADDR (void*)(0x6000000000000000UL)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_SHARED)
#endif

#include <ci/config.h>

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

#if !USE_INTEL_CPU_CYCLES
    if (timing == PERF_CPU_CYCLE) {
        RETURN_ON_ERROR(-0xffff, "Please enable compilation option:USE_INTEL_CPU_CYCLES to use cpu cycle timing.");
    }
#endif

    int ret = 0;
    struct timespec clk_ts, clk_te;
    uint64_t tsc_ts, tsc_te;
    uint64_t iter_per_iter = (bytes_per_iter + buffer_size - 1) / buffer_size;

    // STEP 1 - validate/allocate the buffer
    void* buf = buffer;
#ifdef USE_HUGEPAGE
    int fd = -1;
#endif
    if(buf == NULL) {
#if USE_HUGEPAGE
        fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
        if(fd < 0) {
            perror("Open file failed. Is hugetlbfs mounted?");
            exit(1);
        }
        buf = mmap(ADDR, buffer_size, PROTECTION, FLAGS, fd, 0);
        if(buf == MAP_FAILED) {
            perror("mmap");
            printf("errno=%d\n", errno);
            unlink(FILE_NAME);
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
            ret = clock_gettime(CLOCK_REALTIME, &clk_ts);
            RETURN_ON_ERROR(ret, "clock_gettime");
        } else if (timing == RDTSC) {
            tsc_ts = rdtsc();
        }

        lpcs.start_perf_events();

        // STEP 5 - run experiment
        if(is_write) {
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
#if HAS_AVX2 || HAS_AVX
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
#elif HAS_AVX512
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
        } else {
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
#if HAS_AVX2 || HAS_AVX
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
#elif HAS_AVX512
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
        }

        // STEP 6 - end the timer
        if (timing == CLOCK_GETTIME) {
            ret = clock_gettime(CLOCK_REALTIME, &clk_te);
            RETURN_ON_ERROR(ret, "clock_gettime");
        } else if (timing == RDTSC) {
            tsc_te = rdtsc();
        }

        lpcs.stop_perf_events();

        // STEP 7 - calculate throguhput
        if (timing == PERF_CPU_CYCLE) {
            results[iter] = (iter_per_iter)*buffer_size / static_cast<double>(lpcs.get().at("cpu_cycles(pf)"));
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
        close(fd);
        unlink(FILE_NAME);
#else
        free(buf);
#endif
    }

    return 0;
}

const sequential_throughput_test_schedule_t default_sequential_throughput_test_schedule = {
    {4096,0x8000000,32},
    {8192,0x8000000,32},
    {9216,0x8000000,32},
    {10240,0x8000000,32},
    {11264,0x8000000,32},
    {12288,0x8000000,32},
    {13312,0x8000000,32},
    {14336,0x8000000,32},
    {15360,0x8000000,32},
    {16384,0x8000000,32},
    {17408,0x8000000,32},
    {18432,0x8000000,32},
    {19456,0x8000000,32},
    {20480,0x8000000,32},
    {21504,0x8000000,32},
    {22528,0x8000000,32},
    {23552,0x8000000,32},
    {24576,0x8000000,32},
    {25600,0x8000000,32},
    {26624,0x8000000,32},
    {27648,0x8000000,32},
    {28672,0x8000000,32},
    {29696,0x8000000,32},
    {30720,0x8000000,32},
    {31744,0x8000000,32},
    {32768,0x8000000,32},
    {33792,0x8000000,32},
    {34816,0x8000000,32},
    {35840,0x8000000,32},
    {36864,0x8000000,32},
    {37888,0x8000000,32},
    {38912,0x8000000,32},
    {39936,0x8000000,32},
    {40960,0x8000000,32},
    {41984,0x8000000,32},
    {43008,0x8000000,32},
    {44032,0x8000000,32},
    {45056,0x8000000,32},
    {46080,0x8000000,32},
    {47104,0x8000000,32},
    {48128,0x8000000,32},
    {49152,0x8000000,32},
    {50176,0x8000000,32},
    {51200,0x8000000,32},
    {52224,0x8000000,32},
    {53248,0x8000000,32},
    {55296,0x8000000,32},
    {56320,0x8000000,32},
    {57344,0x8000000,32},
    {58368,0x8000000,32},
    {59392,0x8000000,32},
    {60416,0x8000000,32},
    {61440,0x8000000,32},
    {63488,0x8000000,32},
    {64512,0x8000000,32},
    {65536,0x8000000,32},
    {66560,0x8000000,32},
    {68608,0x8000000,32},
    {69632,0x8000000,32},
    {70656,0x8000000,32},
    {72704,0x8000000,32},
    {73728,0x8000000,32},
    {75776,0x8000000,32},
    {76800,0x8000000,32},
    {77824,0x8000000,32},
    {79872,0x8000000,32},
    {81920,0x8000000,32},
    {82944,0x8000000,32},
    {84992,0x8000000,32},
    {86016,0x8000000,32},
    {88064,0x8000000,32},
    {90112,0x8000000,32},
    {92160,0x8000000,32},
    {93184,0x8000000,32},
    {95232,0x8000000,32},
    {97280,0x8000000,32},
    {99328,0x8000000,32},
    {101376,0x8000000,32},
    {103424,0x8000000,32},
    {105472,0x8000000,32},
    {107520,0x8000000,32},
    {109568,0x8000000,32},
    {111616,0x8000000,32},
    {113664,0x8000000,32},
    {116736,0x8000000,32},
    {118784,0x8000000,32},
    {120832,0x8000000,32},
    {123904,0x8000000,32},
    {125952,0x8000000,32},
    {128000,0x8000000,32},
    {131072,0x8000000,32},
    {134144,0x8000000,32},
    {136192,0x8000000,32},
    {139264,0x8000000,32},
    {142336,0x8000000,32},
    {144384,0x8000000,32},
    {147456,0x8000000,32},
    {150528,0x8000000,32},
    {153600,0x8000000,32},
    {156672,0x8000000,32},
    {159744,0x8000000,32},
    {162816,0x8000000,32},
    {165888,0x8000000,32},
    {169984,0x8000000,32},
    {173056,0x8000000,32},
    {176128,0x8000000,32},
    {180224,0x8000000,32},
    {183296,0x8000000,32},
    {187392,0x8000000,32},
    {190464,0x8000000,32},
    {194560,0x8000000,32},
    {198656,0x8000000,32},
    {202752,0x8000000,32},
    {206848,0x8000000,32},
    {210944,0x8000000,32},
    {215040,0x8000000,32},
    {219136,0x8000000,32},
    {223232,0x8000000,32},
    {228352,0x8000000,32},
    {232448,0x8000000,32},
    {237568,0x8000000,32},
    {241664,0x8000000,32},
    {246784,0x8000000,32},
    {251904,0x8000000,32},
    {257024,0x8000000,32},
    {262144,0x8000000,32},
    {267264,0x8000000,32},
    {272384,0x8000000,32},
    {278528,0x8000000,32},
    {283648,0x8000000,32},
    {289792,0x8000000,32},
    {294912,0x8000000,32},
    {301056,0x8000000,32},
    {307200,0x8000000,32},
    {313344,0x8000000,32},
    {319488,0x8000000,32},
    {325632,0x8000000,32},
    {332800,0x8000000,32},
    {338944,0x8000000,32},
    {346112,0x8000000,32},
    {352256,0x8000000,32},
    {359424,0x8000000,32},
    {366592,0x8000000,32},
    {374784,0x8000000,32},
    {381952,0x8000000,32},
    {389120,0x8000000,32},
    {397312,0x8000000,32},
    {405504,0x8000000,32},
    {413696,0x8000000,32},
    {421888,0x8000000,32},
    {430080,0x8000000,32},
    {438272,0x8000000,32},
    {447488,0x8000000,32},
    {456704,0x8000000,32},
    {464896,0x8000000,32},
    {475136,0x8000000,32},
    {484352,0x8000000,32},
    {493568,0x8000000,32},
    {503808,0x8000000,32},
    {514048,0x8000000,32},
    {524288,0x8000000,32},
    {534528,0x8000000,32},
    {545792,0x8000000,32},
    {556032,0x8000000,32},
    {567296,0x8000000,32},
    {578560,0x8000000,32},
    {589824,0x8000000,32},
    {602112,0x8000000,32},
    {614400,0x8000000,32},
    {626688,0x8000000,32},
    {638976,0x8000000,32},
    {651264,0x8000000,32},
    {664576,0x8000000,32},
    {677888,0x8000000,32},
    {691200,0x8000000,32},
    {705536,0x8000000,32},
    {719872,0x8000000,32},
    {734208,0x8000000,32},
    {748544,0x8000000,32},
    {763904,0x8000000,32},
    {779264,0x8000000,32},
    {794624,0x8000000,32},
    {809984,0x8000000,32},
    {826368,0x8000000,32},
    {842752,0x8000000,32},
    {860160,0x8000000,32},
    {877568,0x8000000,32},
    {894976,0x8000000,32},
    {912384,0x8000000,32},
    {930816,0x8000000,32},
    {949248,0x8000000,32},
    {968704,0x8000000,32},
    {988160,0x8000000,32},
    {1007616,0x8000000,32},
    {1028096,0x8000000,32},
    {1048576,0x8000000,32},
    {1069056,0x8000000,32},
    {1090560,0x8000000,32},
    {1112064,0x8000000,32},
    {1134592,0x8000000,32},
    {1157120,0x8000000,32},
    {1180672,0x8000000,32},
    {1204224,0x8000000,32},
    {1227776,0x8000000,32},
    {1252352,0x8000000,32},
    {1277952,0x8000000,32},
    {1303552,0x8000000,32},
    {1329152,0x8000000,32},
    {1355776,0x8000000,32},
    {1383424,0x8000000,32},
    {1411072,0x8000000,32},
    {1438720,0x8000000,32},
    {1467392,0x8000000,32},
    {1497088,0x8000000,32},
    {1526784,0x8000000,32},
    {1557504,0x8000000,32},
    {1588224,0x8000000,32},
    {1619968,0x8000000,32},
    {1652736,0x8000000,32},
    {1685504,0x8000000,32},
    {1719296,0x8000000,32},
    {1754112,0x8000000,32},
    {1788928,0x8000000,32},
    {1824768,0x8000000,32},
    {1861632,0x8000000,32},
    {1898496,0x8000000,32},
    {1936384,0x8000000,32},
    {1975296,0x8000000,32},
    {2015232,0x8000000,32},
    {2055168,0x8000000,32},
    {2096128,0x8000000,32},
    {2138112,0x8000000,32},
    {2181120,0x8000000,32},
    {2224128,0x8000000,32},
    {2269184,0x8000000,32},
    {2314240,0x8000000,32},
    {2360320,0x8000000,32},
    {2407424,0x8000000,32},
    {2455552,0x8000000,32},
    {2504704,0x8000000,32},
    {2554880,0x8000000,32},
    {2606080,0x8000000,32},
    {2658304,0x8000000,32},
    {2711552,0x8000000,32},
    {2765824,0x8000000,32},
    {2821120,0x8000000,32},
    {2877440,0x8000000,32},
    {2934784,0x8000000,32},
    {2994176,0x8000000,32},
    {3053568,0x8000000,32},
    {3115008,0x8000000,32},
    {3177472,0x8000000,32},
    {3240960,0x8000000,32},
    {3305472,0x8000000,32},
    {3372032,0x8000000,32},
    {3438592,0x8000000,32},
    {3508224,0x8000000,32},
    {3577856,0x8000000,32},
    {3649536,0x8000000,32},
    {3722240,0x8000000,32},
    {3796992,0x8000000,32},
    {3872768,0x8000000,32},
    {3950592,0x8000000,32},
    {4029440,0x8000000,32},
    {4110336,0x8000000,32},
    {4192256,0x8000000,32},
    {4276224,0x8000000,32},
    {4361216,0x8000000,32},
    {4449280,0x8000000,32},
    {4537344,0x8000000,32},
    {4628480,0x8000000,32},
    {4720640,0x8000000,32},
    {4815872,0x8000000,32},
    {4912128,0x8000000,32},
    {5010432,0x8000000,32},
    {5109760,0x8000000,32},
    {5212160,0x8000000,32},
    {5316608,0x8000000,32},
    {5423104,0x8000000,32},
    {5531648,0x8000000,32},
    {5642240,0x8000000,32},
    {5754880,0x8000000,32},
    {5870592,0x8000000,32},
    {5987328,0x8000000,32},
    {6107136,0x8000000,32},
    {6228992,0x8000000,32},
    {6353920,0x8000000,32},
    {6480896,0x8000000,32},
    {6610944,0x8000000,32},
    {6743040,0x8000000,32},
    {6878208,0x8000000,32},
    {7015424,0x8000000,32},
    {7155712,0x8000000,32},
    {7299072,0x8000000,32},
    {7444480,0x8000000,32},
    {7593984,0x8000000,32},
    {7745536,0x8000000,32},
    {7900160,0x8000000,32},
    {8057856,0x8000000,32},
    {8219648,0x8000000,32},
    {8383488,0x8000000,32},
    {8551424,0x8000000,32},
    {8722432,0x8000000,32},
    {8897536,0x8000000,32},
    {9074688,0x8000000,32},
    {9256960,0x8000000,32},
    {9441280,0x8000000,32},
    {9630720,0x8000000,32},
    {9823232,0x8000000,32},
    {10019840,0x8000000,32},
    {10219520,0x8000000,32},
    {10424320,0x8000000,32},
    {10633216,0x8000000,32},
    {10845184,0x8000000,32},
    {11062272,0x8000000,32},
    {11283456,0x8000000,32},
    {11509760,0x8000000,32},
    {11739136,0x8000000,32},
    {11974656,0x8000000,32},
    {12214272,0x8000000,32},
    {12457984,0x8000000,32},
    {12706816,0x8000000,32},
    {12961792,0x8000000,32},
    {13220864,0x8000000,32},
    {13485056,0x8000000,32},
    {13754368,0x8000000,32},
    {14029824,0x8000000,32},
    {14310400,0x8000000,32},
    {14596096,0x8000000,32},
    {14888960,0x8000000,32},
    {15185920,0x8000000,32},
    {15490048,0x8000000,32},
    {15800320,0x8000000,32},
    {16115712,0x8000000,32},
    {16438272,0x8000000,32},
    {16766976,0x8000000,32},
    {17101824,0x8000000,32},
    {17443840,0x8000000,32},
    {17793024,0x8000000,32},
    {18149376,0x8000000,32},
    {18511872,0x8000000,32},
    {18882560,0x8000000,32},
    {19259392,0x8000000,32},
    {19645440,0x8000000,32},
    {20037632,0x8000000,32},
    {20439040,0x8000000,32},
    {20847616,0x8000000,32},
    {21264384,0x8000000,32},
    {21689344,0x8000000,32},
    {22123520,0x8000000,32},
    {22565888,0x8000000,32},
    {23017472,0x8000000,32},
    {23477248,0x8000000,32},
    {23947264,0x8000000,32},
    {24426496,0x8000000,32},
    {24914944,0x8000000,32},
    {25412608,0x8000000,32},
    {25921536,0x8000000,32},
    {26439680,0x8000000,32},
    {26968064,0x8000000,32},
    {27507712,0x8000000,32},
    {28057600,0x8000000,32},
    {28618752,0x8000000,32},
    {29191168,0x8000000,32},
    {29775872,0x8000000,32},
    {30370816,0x8000000,32},
    {30978048,0x8000000,32},
    {31597568,0x8000000,32},
    {32229376,0x8000000,32},
    {32874496,0x8000000,32},
    {33531904,0x8000000,32},
    {34202624,0x8000000,32},
    {34886656,0x8000000,32},
    {35584000,0x8000000,32},
    {36295680,0x8000000,32},
    {37021696,0x8000000,32},
    {37762048,0x8000000,32},
    {38517760,0x8000000,32},
    {39287808,0x8000000,32},
    {40074240,0x8000000,32},
    {40875008,0x8000000,32},
    {41693184,0x8000000,32},
    {42526720,0x8000000,32},
    {43376640,0x8000000,32},
    {44244992,0x8000000,32},
    {45129728,0x8000000,32},
    {46031872,0x8000000,32},
    {46952448,0x8000000,32},
    {47891456,0x8000000,32},
    {48849920,0x8000000,32},
    {49826816,0x8000000,32},
    {50823168,0x8000000,32},
    {51840000,0x8000000,32},
    {52876288,0x8000000,32},
    {53934080,0x8000000,32},
    {55012352,0x8000000,32},
    {56113152,0x8000000,32},
    {57235456,0x8000000,32},
    {58380288,0x8000000,32},
    {59547648,0x8000000,32},
    {60738560,0x8000000,32},
    {61953024,0x8000000,32},
    {63192064,0x8000000,32},
    {64455680,0x8000000,32},
    {65744896,0x8000000,32},
    {67059712,0x8000000,32},
    {68401152,0x8000000,32},
    {69769216,0x8000000,32},
    {71164928,0x8000000,32},
    {72588288,0x8000000,32},
    {74040320,0x8000000,32},
    {75521024,0x8000000,32},
    {77031424,0x8000000,32},
    {78571520,0x8000000,32},
    {80143360,0x8000000,32},
    {81745920,0x8000000,32},
    {83381248,0x8000000,32},
    {85048320,0x8000000,32},
    {86749184,0x8000000,32},
    {88484864,0x8000000,32},
    {90254336,0x8000000,32},
    {92059648,0x8000000,32},
    {93900800,0x8000000,32},
    {95778816,0x8000000,32},
    {97693696,0x8000000,32},
    {99647488,0x8000000,32},
    {101641216,0x8000000,32},
    {103673856,0x8000000,32},
    {105747456,0x8000000,32},
    {107862016,0x8000000,32},
    {110019584,0x8000000,32},
    {112219136,0x8000000,32},
    {114463744,0x8000000,32},
    {116753408,0x8000000,32},
    {119088128,0x8000000,32},
    {121469952,0x8000000,32},
    {123899904,0x10000000,8},
    {126377984,0x10000000,8},
    {128905216,0x10000000,8},
    {131483648,0x10000000,8},
    {134113280,0x10000000,8},
    {136795136,0x10000000,8},
    {139531264,0x10000000,8},
    {142321664,0x10000000,8},
    {145168384,0x10000000,8},
    {148071424,0x10000000,8},
    {151032832,0x10000000,8},
    {154053632,0x10000000,8},
    {157134848,0x10000000,8},
    {160277504,0x10000000,8},
    {163482624,0x10000000,8},
    {166752256,0x10000000,8},
    {170087424,0x10000000,8},
    {173489152,0x10000000,8},
    {176958464,0x10000000,8},
    {180498432,0x10000000,8},
    {184108032,0x10000000,8},
    {187790336,0x10000000,8},
    {191546368,0x10000000,8},
    {195377152,0x10000000,8},
    {199284736,0x10000000,8},
    {203270144,0x10000000,8},
    {207335424,0x10000000,8},
    {211482624,0x10000000,8},
    {215711744,0x10000000,8},
    {220025856,0x10000000,8},
    {224427008,0x10000000,8},
    {228915200,0x10000000,8},
    {233493504,0x10000000,8},
    {238162944,0x10000000,8},
    {242926592,0x10000000,8},
    {247785472,0x10000000,8},
    {252740608,0x10000000,8},
    {257796096,0x10000000,8},
    {262951936,0x10000000,8},
    {268210176,0x10000000,8},
    {273574912,0x10000000,8},
};

extern volatile int32_t sequential_throughput_cliffs(
    const sequential_throughput_test_schedule_t& schedule,
    sequential_throughput_test_result_t& result,
    timing_mechanism_t timing) {

#if !USE_INTEL_CPU_CYCLES
    if (timing == PERF_CPU_CYCLE) {
        RETURN_ON_ERROR(-0xffff, "Please enable compilation option:USE_INTEL_CPU_CYCLES to use cpu cycle timing.");
    }
#endif

    // STEP 1: get parameters
    uint64_t len_schedule = schedule.size();
    if (schedule.empty()) {
        // empty schedule
        RETURN_ON_ERROR(-1,"Error: empty schedule.");
    }
    uint64_t total_buffer_size = std::get<0>(schedule.back());
    void* buf = nullptr;
    const uint64_t page_size = getpagesize();
    total_buffer_size = (total_buffer_size + page_size - 1)/page_size*page_size;

    // STEP 2: allocate space
#ifdef USE_HUGEPAGE
    int hp_fd = -1;
#endif

#if USE_HUGEPAGE
    fd = open(FILE_NAME, O_CREAT | O_RDWR, 0755);
    if(fd < 0) {
        RETURN_ON_ERROR(-2, "Open hugrpage file failed, Is hugetlbfs mounted at " FILE_NAME " ?");
    }
    buf = mmap(ADDR, total_buffer_size, PROTECTION, FLAGS, fd, 0);
    if(buf == MAP_FAILED) {
        perror("mmap");
        fprintf("errno=%d\n", errno);
        unlink(FILE_NAME);
        return -2;
    }
#else
    if (posix_memalign(&buf, page_size, total_buffer_size)) {
        RETURN_ON_ERROR(-3, "posix_memalign.");
    }
#endif

    // STEP 3: do tests
    result.resize(len_schedule);
    for (uint64_t i=0;i<len_schedule;i++) {
        uint64_t buffer_size = std::get<0>(schedule[i]);
        uint64_t bytes_per_iter = std::get<1>(schedule[i]);
        uint64_t num_iter = std::get<2>(schedule[i]);
        uint64_t num_iter_warmup = num_iter/6;
        if (num_iter_warmup == 0) num_iter_warmup = 1;
        if (num_iter_warmup > 5) num_iter_warmup = 5;

        double iter_results[num_iter];
        auto lpcs = std::optional<std::vector<std::map<std::string, long long>>>();

        double avg_read_thp = 0.0f,read_thp_dev = 0.0f,avg_write_thp = 0.0f,write_thp_dev = 0.0f;
        // read throughput
        if (sequential_throughput(buf,buffer_size,num_iter,iter_results,lpcs,timing,0,bytes_per_iter)) {
            RETURN_ON_ERROR(-4, "Read test with sequential_throughput() failed.");
        } else {
            avg_read_thp = average(num_iter, iter_results);
            read_thp_dev = deviation(num_iter, iter_results);
        }
        // write throughput
        if (sequential_throughput(buf,buffer_size,num_iter,iter_results,lpcs,timing,1,bytes_per_iter)) {
            RETURN_ON_ERROR(-4, "Write test with sequential_throughput() failed.");
        } else {
            avg_write_thp = average(num_iter, iter_results);
            write_thp_dev = deviation(num_iter, iter_results);
        }

        result[i] = std::make_tuple(avg_read_thp,read_thp_dev,avg_write_thp,write_thp_dev);
    }

    // STEP 4: deallocate space
#if USE_HUGEPAGE
        munmap(buf, total_buffer_size);
        close(fd);
        unlink(FILE_NAME);
#else
        free(buf);
#endif
    return 0;
}

}//namespace cacheinspector
