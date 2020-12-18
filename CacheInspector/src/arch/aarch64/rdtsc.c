/*                                                                             
 * Enable user-mode ARM performance counter access.                            
 */                                                                           
#include <linux/kernel.h>                                                      
#include <linux/module.h>                                                      
#include <linux/smp.h>                                                         
                                                                                
 
#define PERF_DEF_OPTS       (1 | 16)                                                                       
#define PERF_OPT_RESET_CYCLES   (2 | 4)                                                                  
#define PERF_OPT_DIV64      (8)                                                                          
#define ARMV8_PMCR_MASK         0x3f                                                                    
#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */                                      
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */                                       
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */                                      
#define ARMV8_PMCR_D            (1 << 3) /* CCNT counts every 64th cpu cycle */                         
#define ARMV8_PMCR_X            (1 << 4) /* Export to ETM */                                            
#define ARMV8_PMCR_DP           (1 << 5) /* Disable CCNT if non-invasive debug*/                        
#define ARMV8_PMCR_LC           (1 << 6) /* Cycle Counter 64bit overflow*/
#define ARMV8_PMCR_N_SHIFT      11       /* Number of counters supported */                             
#define ARMV8_PMCR_N_MASK       0x1f                                                                    
                                                                                                         
#define ARMV8_PMUSERENR_EN_EL0  (1 << 0) /* EL0 access enable */                                        
#define ARMV8_PMUSERENR_CR      (1 << 2) /* Cycle counter read enable */                                
#define ARMV8_PMUSERENR_ER      (1 << 3) /* Event counter read enable */                                
                                                                                                         
static inline u32 armv8pmu_pmcr_read(void)                                                              
{                                                                                                       
        u64 val=0;                                                                                      
        asm volatile("mrs %0, pmcr_el0" : "=r" (val));                                                  
        return (u32)val;                                                                                
}                                                                                                       
static inline void armv8pmu_pmcr_write(u32 val)                                                         
{                                                                                                       
        val &= ARMV8_PMCR_MASK;                                                                         
        isb();                                                                                          
        asm volatile("msr pmcr_el0, %0" : : "r" ((u64)val));                                            
}       
 
static inline  long long armv8_read_CNTPCT_EL0(void)
{
   long long val;
   asm volatile("mrs %0, CNTVCT_EL0" : "=r" (val));
 
   return val;
}
 
                                                                                                         
static void                                                                                            
enable_cpu_counters(void* data)                                                                         
{                                                                                                       
    asm volatile("msr pmuserenr_el0, %0" : : "r"(0xf));
    armv8pmu_pmcr_write(ARMV8_PMCR_LC|ARMV8_PMCR_E);                                                      
        asm volatile("msr PMCNTENSET_EL0, %0" :: "r" ((u32)(1<<31)));
    armv8pmu_pmcr_write(armv8pmu_pmcr_read() | ARMV8_PMCR_E|ARMV8_PMCR_LC);   
        printk("\nCPU:%d ", smp_processor_id());
}                                                                                                       
                                                                                                         
static void                                                                                            
disable_cpu_counters(void* data)                                                                        
{                                                                                                       
    printk(KERN_INFO "\ndisabling user-mode PMU access on CPU #%d",                       
    smp_processor_id());                                                                                   
                                                                                                         
    /* Program PMU and disable all counters */                                                            
        armv8pmu_pmcr_write(armv8pmu_pmcr_read() |~ARMV8_PMCR_E);                                              
    asm volatile("msr pmuserenr_el0, %0" : : "r"((u64)0));                                                 
                                                                                                         
}                                                                                                       
                                                                                                         
static int __init                                                                                       
init(void)                                                                                              
{                                                                       
    u64 cval;
        u32 val;
 
        isb();
        asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(cval));
        printk("\nCPU Cycle count:%llu \n", cval);
        asm volatile("mrs %0, PMCNTENSET_EL0" : "=r"(val));
        printk("PMCNTENSET_EL0:%X ", val);
        asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
        printk("\nPMCR_EL0 Register:%X ", val);
 
        on_each_cpu(enable_cpu_counters, NULL, 1);                                                             
        printk(KERN_INFO "Enable Access PMU Initialized");                                                       
    return 0;                                                                                              
}                                                                                                       
                                                                                                         
static void __exit                                                                                      
fini(void)                                                                                              
{                                                                                                       
    on_each_cpu(disable_cpu_counters, NULL, 1);                                                            
    printk(KERN_INFO "Access PMU Disabled");                                                          
}                                                                                                       
                                                                                                         
module_init(init);                                                                                      
module_exit(fini);
