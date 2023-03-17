// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _CPU_H
#define _CPU_H

#include <platform.h>
#include <stdint.h>

#ifdef __JETSON_AGX_XAVIER
  // files
  #define CUR_FREQ_CPU_FILE "/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_cur_freq"
  #define CORE_FREQ_CPU_FILE "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq"
  #define AVAIL_FREQ_CPU_FILE "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies"
  // define CPU hardware
  #define NUM_CORES_CPU 8
  #define NUM_COUNTERS_CPU 3 // per core
#else
  #error "Platform not supported."
#endif

typedef uint32_t cpu_event_t;

typedef struct {
  unsigned int num_events_core;
  cpu_event_t *event;
} cpu_core_events_t;

typedef struct {
  uint32_t frequency;
  unsigned int num_cores;
  cpu_core_events_t *core;
} cpu_events_freq_config_t;

typedef struct {
  unsigned int num_freqs;
  cpu_events_freq_config_t *cpu_events_freq_config;
} cpu_events_config_t;

uint32_t setup_cpu();
void deinit_cpu();

void cpu_events_from_cli(cpu_event_t *events, unsigned int num_events);
void cpu_events_from_config(char *config_file);
void parse_cpu_events_json(char *config_file, cpu_events_config_t *events_config);

uint32_t get_cpu_freq();
uint32_t clip_cpu_freq(uint32_t freq);

#endif // _CPU_H
