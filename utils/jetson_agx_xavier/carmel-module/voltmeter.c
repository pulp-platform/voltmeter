// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Thomas Benz, ETH Zurich <tbenz@iis.ee.ethz.ch>
//         Björn Forsberg, ETH Zurich <bjoernf@iis.ee.ethz.ch>

#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/smp.h>

#define CPUACTLR_EL1_DIS_L1D_PREF        (((u64)1)<<56)
#define CPUACTLR_EL1_DIS_L12D_BOUND_PREF (((u64)1)<<47)
#define CPUACTLR_EL1_DIS_L1I_PREF        (((u64)1)<<32)
#define CPUACTLR_EL1_DIS_L2TLB_PREF      (((u64)1)<<21)

#define CPUECTLR_EL1_DIS_TBLWLK          (((u64)1)<<38)
#define CPUECTLR_EL1_L2I_PREF_DIST2      (((u64)1)<<36)
#define CPUECTLR_EL1_L2I_PREF_DIST1      (((u64)1)<<35)
#define CPUECTLR_EL1_L2D_PREF_DIST2      (((u64)1)<<33)
#define CPUECTLR_EL1_L2D_PREF_DIST1      (((u64)1)<<32)

#define ARMV8_PMCR_MASK                  0x3f
#define ARMV8_PMCR_E                     (1 << 0) /*  Enable all counters */
#define ARMV8_PMCR_P                     (1 << 1) /*  Reset all counters */
#define ARMV8_PMCR_C                     (1 << 2) /*  Cycle counter reset */
#define ARMV8_PMCR_D                     (1 << 3) /*  CCNT counts every 64th cpu cycle */
#define ARMV8_PMCR_X                     (1 << 4) /*  Export to ETM */
#define ARMV8_PMCR_DP                    (1 << 5) /*  Disable CCNT if non-invasive debug*/
#define ARMV8_PMCR_N_SHIFT               11       /*  Number of counters supported */
#define ARMV8_PMCR_N_MASK                0x1f

#define ARMV8_PMUSERENR_EN_EL0           (1 << 0) /*  EL0 access enable */
#define ARMV8_PMUSERENR_CR               (1 << 2) /*  Cycle counter read enable */
#define ARMV8_PMUSERENR_ER               (1 << 3) /*  Event counter read enable */

#define ARMV8_PMCNTENSET_EL0_ENABLE      (1<<31) /* Enable Perf count reg */

#define PERF_DEF_OPTS                    (1 | 16)
#define PERF_OPT_RESET_CYCLES            (2 | 4)
#define PERF_OPT_DIV64                   (8)

MODULE_LICENSE("GPL");
MODULE_AUTHOR(".");
MODULE_DESCRIPTION("Enable userspace access to ARM PMU");

#define MODULE_NAME "voltmeter"

static inline u32 armv8pmu_pmcr_read(void) {
  u64 val=0;
  asm volatile("mrs %0, pmcr_el0" : "=r" (val));
  return (u32)val;
}

static inline void armv8pmu_pmcr_write(u32 val) {
  val &= ARMV8_PMCR_MASK;
  isb();
  asm volatile("msr pmcr_el0, %0" : : "r" ((u64)val));
}

static void enable_cpu_counters(void* data) {
  printk(KERN_INFO "[" MODULE_NAME "] enabling user-mode PMU access on CPU #%d", smp_processor_id());

#if __aarch64__
  /*  Enable user-mode access to counters. */
  asm volatile("msr pmuserenr_el0, %0" : : "r"((u64)ARMV8_PMUSERENR_EN_EL0|ARMV8_PMUSERENR_ER|ARMV8_PMUSERENR_CR));
  /*  Initialize & Reset PMNC: C and P bits. */
  armv8pmu_pmcr_write(ARMV8_PMCR_P | ARMV8_PMCR_C);
  /* G4.4.11
   * PMINTENSET, Performance Monitors Interrupt Enable Set register */
  /* cycle counter overflow interrupt request is disabled */
  asm volatile("msr pmintenset_el1, %0" : : "r" ((u64)(0 << 31)));
  /*   Performance Monitors Count Enable Set register bit 30:0 disable, 31 enable */
  asm volatile("msr pmcntenset_el0, %0" : : "r" (ARMV8_PMCNTENSET_EL0_ENABLE));
  /* start*/
  armv8pmu_pmcr_write(armv8pmu_pmcr_read() | ARMV8_PMCR_E);
#elif defined(__ARM_ARCH_7A__)
  /* Enable user-mode access to counters. */
  asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(1));
  /* Program PMU and enable all counters */
  asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(PERF_DEF_OPTS));
  asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
#else
#error Unsupported Architecture
#endif
}

static void disable_cpu_counters(void* data) {
  printk(KERN_INFO "[" MODULE_NAME "] disabling user-mode PMU access on CPU #%d", smp_processor_id());

#if __aarch64__
  /*  Performance Monitors Count Enable Set register bit 31:0 disable, 1 enable */
  asm volatile("msr pmcntenset_el0, %0" : : "r" (0<<31));
  /*  Note above statement does not really clearing register...refer to doc */
  /*  Program PMU and disable all counters */
  armv8pmu_pmcr_write(armv8pmu_pmcr_read() |~ARMV8_PMCR_E);
  /*  disable user-mode access to counters. */
  asm volatile("msr pmuserenr_el0, %0" : : "r"((u64)0));
#elif defined(__ARM_ARCH_7A__)
  /* Program PMU and disable all counters */
  asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0));
  asm volatile("mcr p15, 0, %0, c9, c12, 2" :: "r"(0x8000000f));
  /* Disable user-mode access to counters. */
  asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(0));
#else
#error Unsupported Architecture
#endif
}

static void get_cache_info(void) {
  u64 val64;
  asm volatile("mrs %0, CLIDR_EL1" : "=r" (val64));
  printk(KERN_INFO "Cache types (CLIDR_EL1): 0x%016llX\n", val64);
  /* Level 1 cache info */
  val64 = 0;
  asm volatile("msr CSSELR_EL1, %0" :: "r" (val64));
  asm volatile("mrs %0, CCSIDR_EL1" : "=r" (val64));
  printk(KERN_INFO "Level 1 I size (CCSIDR_EL1): 0x%016llX\n", val64);
  /**/
  val64 = 1;
  asm volatile("msr CSSELR_EL1, %0" :: "r" (val64));
  asm volatile("mrs %0, CCSIDR_EL1" : "=r" (val64));
  printk(KERN_INFO "Level 1 D size (CCSIDR_EL1): 0x%016llX\n", val64);
  /**/
  val64 = 2;
  asm volatile("msr CSSELR_EL1, %0" :: "r" (val64));
  asm volatile("mrs %0, CCSIDR_EL1" : "=r" (val64));
  printk(KERN_INFO "Level 2 U size (CCSIDR_EL1): 0x%016llX\n", val64);
}

static int __init voltmeter_init(void) {
  get_cache_info();
  on_each_cpu(enable_cpu_counters, NULL, 1);
  printk(KERN_INFO "[" MODULE_NAME "] initialized");
  return 0; // Non-zero return means that the module couldn't be loaded.
}

static void __exit voltmeter_cleanup(void) {
  on_each_cpu(disable_cpu_counters, NULL, 1);
  printk(KERN_INFO "[" MODULE_NAME "] unloaded");
}

module_init(voltmeter_init);
module_exit(voltmeter_cleanup);
