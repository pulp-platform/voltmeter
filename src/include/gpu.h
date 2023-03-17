// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _GPU_H
#define _GPU_H

#include <platform.h>
#include <stdint.h>
#ifdef __JETSON_AGX_XAVIER
#include <cuda_runtime_api.h>
#include <cupti_events.h>
#endif

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

#ifdef __JETSON_AGX_XAVIER
typedef CUpti_EventID gpu_event_t;
#else
typedef uint32_t gpu_event_t;
#endif

typedef struct {
  uint32_t frequency;
  unsigned int num_events;
  gpu_event_t *event;
#ifdef __JETSON_AGX_XAVIER
  CUpti_EventGroupSets *event_group_sets;
#endif
} gpu_events_freq_config_t;

typedef struct {
  unsigned int num_freqs;
  gpu_events_freq_config_t *gpu_events_freq_config;
} gpu_events_config_t;

uint32_t setup_gpu();
void deinit_gpu();

uint32_t gpu_events_all();
uint32_t gpu_events_from_cli(gpu_event_t *events, unsigned int num_events);
uint32_t gpu_events_from_config(char *config_file);
void parse_gpu_events_json(char *config_file, gpu_events_config_t *events_config);

uint32_t get_gpu_freq();
uint32_t clip_gpu_freq(uint32_t freq);

#endif // _GPU_H
