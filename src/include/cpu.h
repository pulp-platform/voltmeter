// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>
//         Thomas Benz, ETH Zurich <tbenz@iis.ee.ethz.ch>
//         Björn Forsberg

#ifndef _CPU_H
#define _CPU_H

// standard includes
#include <stdint.h>

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Macros                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

#ifdef __JETSON_AGX_XAVIER
  // files
  #define CUR_FREQ_CPU_FILE "/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq"
  #define CORE_FREQ_CPU_FILE "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq"
  #define AVAIL_FREQ_CPU_FILE "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"
  // define CPU hardware
  #define NUM_CORES_CPU 8
  #define NUM_COUNTERS_CPU 3 // per core (only configurable counters; then Jetson has 1 more for clock)
  // ARM PMU defines (Carmel SoC)
  #define ARMV8_PMEVTYPER_P              (1 << 31) // EL1 modes filtering bit
  #define ARMV8_PMEVTYPER_U              (1 << 30) // EL0 filtering bit
  #define ARMV8_PMEVTYPER_NSK            (1 << 29) // Non-secure EL1 (kernel) modes filtering bit
  #define ARMV8_PMEVTYPER_NSU            (1 << 28) // Non-secure User mode filtering bit
  #define ARMV8_PMEVTYPER_NSH            (1 << 27) // Non-secure Hyp modes filtering bit
  #define ARMV8_PMEVTYPER_M              (1 << 26) // Secure EL3 filtering bit
  #define ARMV8_PMEVTYPER_MT             (1 << 25) // Multithreading
  #define ARMV8_PMEVTYPER_EVTCOUNT_MASK  0x3ff
#else
  #error "Platform not supported."
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                         Types                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

#ifdef __JETSON_AGX_XAVIER
typedef uint32_t cpu_event_id_t;
typedef uint32_t cpu_counter_t;
#else
#error "Platform not supported."
#endif

#ifdef __JETSON_AGX_XAVIER
typedef struct {
  // configurable PMU perf counters
  unsigned int num_counters_core;
  cpu_event_id_t *event_id;
  cpu_counter_t *counter;
  // ARM PMU clock cycles counter
  uint64_t counter_clk;
} cpu_core_events_t;
#else
#error "Platform not supported."
#endif

typedef struct {
  uint32_t frequency;
  unsigned int num_cores;
  cpu_core_events_t *core;
} cpu_events_freq_config_t;

typedef struct {
  unsigned int num_freqs;
  cpu_events_freq_config_t *cpu_events_freq_config;
} cpu_events_config_t;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                     Declarations                      ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// setup
uint32_t setup_cpu();
void deinit_cpu();

// events parsing
void cpu_events_from_cli(cpu_event_id_t *events, unsigned int num_events);
void cpu_events_from_config(char *config_file);
void parse_cpu_events_json(char *config_file, cpu_events_config_t *events_config);

// performance monitoring unit driver
void enable_pmu_cpu_core(unsigned int core_id);
void disable_pmu_cpu_core();
void read_counters_cpu_core(unsigned int core_id);
void reset_counters_cpu_core();

// helper functions
uint32_t get_cpu_freq();
uint32_t clip_cpu_freq(uint32_t freq);

#endif // _CPU_H
