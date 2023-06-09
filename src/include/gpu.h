// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _GPU_H
#define _GPU_H

// standard includes
#include <stdint.h>
// voltmeter libraries
#include <platform.h>
// third-party libraries
#ifdef __JETSON_AGX_XAVIER
#include <cuda_runtime_api.h>
#include <cupti_events.h>
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Macros                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

#ifdef __JETSON_AGX_XAVIER
  // files
  #define CUR_FREQ_GPU_FILE "/sys/devices/17000000.gv11b/devfreq/17000000.gv11b/cur_freq"
  #define AVAIL_FREQ_GPU_FILE "/sys/devices/17000000.gv11b/devfreq/17000000.gv11b/available_frequencies"
  // statically select GPU 0
  #define CUDA_DEV_NUM 0

  // macros
  #define CHECK_CU_ERROR(err, cufunc)                                                                \
  if (err != CUDA_SUCCESS) {                                                                         \
    printf("%s:%d: error %d for CUDA Driver API function '%s'.\n", __FILE__, __LINE__, err, cufunc); \
    exit(1);                                                                                         \
  }

#define CHECK_CUPTI_ERROR(err, cuptifunc)                                                            \
  if (err != CUPTI_SUCCESS) {                                                                        \
    const char *errstr;                                                                              \
    cuptiGetResultString(err, &errstr);                                                              \
    printf("%s:%d: error %s for CUPTI API function '%s'.\n", __FILE__, __LINE__, errstr, cuptifunc); \
    exit(1);                                                                                         \
  }
#else
  #error "Platform not supported."
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                         Types                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

#ifdef __JETSON_AGX_XAVIER
typedef CUpti_EventID gpu_event_id_t;
typedef uint64_t gpu_counter_t;
#else
#error "Platform not supported."
#endif

typedef struct {
  uint32_t frequency;
  unsigned int num_counters;
  gpu_event_id_t *event_id;
  gpu_counter_t *counter;
  uint32_t freq_read;
#ifdef __JETSON_AGX_XAVIER
  // CUPTI-specific stuff
  CUpti_EventGroupSets *event_group_sets;
  uint32_t *num_events_group;
  uint32_t *num_instances_group;
  size_t *sizes_event_ids_group;
  size_t *sizes_counters_group;
  CUpti_EventID **event_ids_buffer;
  gpu_counter_t **counters_buffer;
#endif
} gpu_events_freq_config_t;

typedef struct {
  unsigned int num_freqs;
  gpu_events_freq_config_t *gpu_events_freq_config;
} gpu_events_config_t;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                     Declarations                      ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// setup
uint32_t setup_gpu(FILE *log_file);
void deinit_gpu();

// events parsing
uint32_t gpu_events_all(FILE *log_file);
uint32_t gpu_events_from_cli(gpu_event_id_t *events, unsigned int num_events, FILE *log_file);
uint32_t gpu_events_from_config(char *config_file, FILE *log_file);
void parse_gpu_events_json(char *config_file, gpu_events_config_t *events_config);

// performance monitoring unit driver
void enable_pmu_gpu(unsigned int set_id);
void disable_pmu_gpu(unsigned int set_id);
void read_counters_gpu(unsigned int set_id);
void reset_counters_gpu();

void read_gpu_freq();

// helper functions
uint32_t get_gpu_freq();
uint32_t clip_gpu_freq(uint32_t freq);
void print_gpu_events(FILE *log_file);
void print_gpu_events_set(FILE *log_file, uint32_t set_id);
void sync_gpu_slave();

#endif // _GPU_H
