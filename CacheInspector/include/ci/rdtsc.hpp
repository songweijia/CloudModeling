/**
 * rdtsc - we use cpu cycle, which is more accurate comparing to the
 */
#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile(".byte 0x0f, 0x31"
                     : "=A"(x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#elif defined(__aarch64__)
/* All counters, including PMCCNTR_EL0, are disabled/enabled */
#define QUADD_ARMV8_PMCR_E      (1 << 0)
/* Reset all event counters, not including PMCCNTR_EL0, to 0 */
#define QUADD_ARMV8_PMCR_P      (1 << 1)
/* Reset PMCCNTR_EL0 to 0 */
#define QUADD_ARMV8_PMCR_C      (1 << 2)
/* Clock divider: PMCCNTR_EL0 counts every clock cycle/every 64 clock cycles */
#define QUADD_ARMV8_PMCR_D      (1 << 3)
/* Export of events is disabled/enabled */
#define QUADD_ARMV8_PMCR_X      (1 << 4)
/* Disable cycle counter, PMCCNTR_EL0 when event counting is prohibited */
#define QUADD_ARMV8_PMCR_DP     (1 << 5)
/* Long cycle count enable */
#define QUADD_ARMV8_PMCR_LC     (1 << 6)
 
#define ARMV8_PMCR_MASK     0x3f     /* Mask for writable bits */
 
static inline unsigned int armv8_pmu_pmcr_read(void)
{
        unsigned int val;
        /* Read Performance Monitors Control Register */
        asm volatile("mrs %0, pmcr_el0" : "=r" (val));
        return val;
}

static __inline__ unsigned long long rdtsc(void) {
    return static_cast<unsigned long long>(armv8_pmu_pmcr_read());
}

static inline void armv8_pmu_pmcr_write(unsigned int val)
{
    asm volatile("msr pmcr_el0, %0" : :"r" (val & ARMV8_PMCR_MASK));
}
 
 
static inline  long long armv8_read_CNTPCT_EL0(void)
{
   long long val;
   asm volatile("mrs %0, CNTVCT_EL0" : "=r" (val));
 
   return val;
}
 
static void enable_all_counters(void)
{
  
    return;
    unsigned int val;
    /* Enable all counters */
    val = armv8_pmu_pmcr_read();
    val |= QUADD_ARMV8_PMCR_E | QUADD_ARMV8_PMCR_X;
    armv8_pmu_pmcr_write(val);
}
 
static void reset_all_counters(void)
{
 
   return ; 
   unsigned int val;
    val = armv8_pmu_pmcr_read();
    val |= QUADD_ARMV8_PMCR_P | QUADD_ARMV8_PMCR_C;
    armv8_pmu_pmcr_write(val);
}
 
static inline unsigned int armv8pmu_pmcr_read(void)
{
    unsigned int val;
    asm volatile("mrs %0, pmcr_el0" : "=r" (val));
    return val;
}
 
#define u32 unsigned int
#define u64 unsigned long long
#define isb()       asm volatile("isb" : : : "memory")
 
static inline u64 arch_counter_get_cntpct(void)
{
    u64 cval;
 
    isb();
        asm volatile("mrs %0, PMCCNTR_EL0" : "+r"(cval));
    return cval;
}

#endif
