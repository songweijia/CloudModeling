#include <time.h>
#include <sched.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "seq_thp.hpp"
#include "util.hpp"

#if USE_HUGEPAGE
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define FILE_NAME "/mnt/huge/hugepagefile"
#define ADDR (void *)(0x8000000000000000UL)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define FLAGS (MAP_SHARED)
#endif



extern int32_t volatile sequential_throughput(
  void * buffer,
  size_t buffer_size,
  uint32_t num_iter,
  double * results,
  uint32_t is_write,
  uint64_t bytes_per_iter,
  uint64_t num_iter_warmup,
  size_t buf_alignment) {

  assert(buffer_size > 0);

  int ret = 0;
  struct timespec ts,te;
  uint64_t iter_per_iter = (bytes_per_iter + buffer_size - 1) / buffer_size;

  // STEP 1 - validate/allocate the buffer
  void * buf = buffer;
#ifdef USE_HUGEPAGE
  int fd = -1;
#endif
  if (buf == NULL) {
#if USE_HUGEPAGE
    fd = open(FILE_NAME, O_CREAT|O_RDWR, 0755);
    if (fd < 0) {
      perror("Open file failed. Is hugetlbfs mounted?");
      exit(1);
    }
    buf = mmap(ADDR, buffer_size, PROTECTION, FLAGS, fd, 0);
    if (buf == MAP_FAILED) {
      perror("mmap");
      unlink(FILE_NAME);
      exit(1);
    }
#else
    ret = posix_memalign(&buf,buf_alignment, buffer_size);
    RETURN_ON_ERROR(ret,"posix_memalign.");
#endif
  }

  // STEP 2 - change scheduler
#if 0
  int max_pri = sched_get_priority_max(SCHED_FIFO);
  RETURN_ON_ERROR(max_pri,"sched_get_priority_max.");
  struct sched_param sch_parm;
  sch_parm.sched_priority = max_pri;
  ret = sched_setscheduler(0,SCHED_FIFO,&sch_parm);
  RETURN_ON_ERROR(ret,"sched_setscheduler.");
#endif
  // STEP 3 - warm up
  while (num_iter_warmup --) {
    for (size_t i=0;i<(buffer_size>>3);i++) {
      ((uint64_t*)buf)[i] = 0;
    }
  }

  while (num_iter --) {
    // STEP 4 - start the timer
    ret = clock_gettime(CLOCK_REALTIME,&ts);
    RETURN_ON_ERROR(ret,"clock_gettime");
  
    // STEP 5 - run experiment
    if (is_write) {
      // -- write
      __asm__ volatile (
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
        : // no output
        : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
        : "rax","rbx","rcx","rdx"
      );
    } else {
      // -- read
      __asm__ volatile (
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
        // start writing
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
        // end of writing
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
        : // no output
        : "m"(buf), "m"(buffer_size), "m"(iter_per_iter)
        : "rax","rbx","rcx","rdx"
      );
 
    }

    // STEP 6 - end the timer
    ret = clock_gettime(CLOCK_REALTIME, &te);
    RETURN_ON_ERROR(ret,"clock_gettime");

    // STEP 7 - calculate throguhput
    results[num_iter] = THROUGHPUT_GBPS( (iter_per_iter)*buffer_size, ts, te);
  }

  // STEP 8 - clean up
  if (!buffer){
#if USE_HUGEPAGE
    munmap(buf,buffer_size);
    close(fd);
    unlink(FILE_NAME);
#else
    free(buf);
#endif
  }

  return 0;
}

