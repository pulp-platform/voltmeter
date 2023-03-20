// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
// third-party libraries
#include <jsmn.h>
#ifdef __JETSON_AGX_XAVIER
#include <cuda_runtime_api.h>
#include <cupti_events.h>
#endif
// voltmeter libraries
#include <platform.h>
#include <helper.h>
#include <cpu.h>

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                      Prototypes                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static void print_cpu_events();
static void free_events_config(cpu_events_config_t *events_config);

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Globals                        ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static cpu_events_freq_config_t cpu_events;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                       Functions                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

uint32_t setup_cpu() {
#ifdef __JETSON_AGX_XAVIER
  // allocate NUM_CORES_CPU cpu_events_freq_config_t.core
  cpu_events.frequency = clip_cpu_freq(get_cpu_freq());
  cpu_events.num_cores = NUM_CORES_CPU;
  cpu_events.core = (cpu_core_events_t *)malloc(NUM_CORES_CPU * sizeof(cpu_core_events_t));
  if (cpu_events.core == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  // for each core in cpu_events, allocate NUM_COUNTERS_CPU cpu_events.core.event
  for (int i = 0; i < NUM_CORES_CPU; i++) {
    // for the Jetson Xavier, the number of PMU counters is fixed
    cpu_events.core[i].num_events_core = NUM_COUNTERS_CPU;
    cpu_events.core[i].event = (cpu_event_t *)malloc(NUM_COUNTERS_CPU * sizeof(cpu_event_t));
    if (cpu_events.core[i].event == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
  }
  printf("CPU cores: %d\n", NUM_CORES_CPU);
  printf("Counters per CPU core: %d\n", NUM_COUNTERS_CPU);
  return cpu_events.frequency;
#else
#error "Platform not supported."
#endif
}

void deinit_cpu(){
#ifdef __JETSON_AGX_XAVIER
  // free cpu_events.num_cores cpu_events.core
  for (int i = 0; i < cpu_events.num_cores; i++) {
    free(cpu_events.core[i].event);
  }
  free(cpu_events.core);
#else
#error "Platform not supported."
#endif
}

void cpu_events_from_cli(cpu_event_t *events, unsigned int num_events){
#ifdef __JETSON_AGX_XAVIER
  // events are expected in the order:
  // core0_event0 core0_event1 core0_event2 core1_event0 core1_event1 ...
  if (num_events != NUM_CORES_CPU * NUM_COUNTERS_CPU) {
    printf("%s:%d: unexpected number of events %d (expected %d).\n", __FILE__, __LINE__, num_events, NUM_COUNTERS_CPU);
    exit(1);
  }
  for (int c = 0; c < cpu_events.num_cores; c++) {
    for (int e = 0; e < cpu_events.core[c].num_events_core; e++) {
      cpu_events.core[c].event[e] = events[c * NUM_COUNTERS_CPU + e];
    }
  }
  print_cpu_events();
#else
#error "Platform not supported."
#endif
}

void cpu_events_from_config(char *config_file) {
  cpu_events_config_t events_config;
  parse_cpu_events_json(config_file, &events_config);

  // find CPU frequency in events_config
  uint32_t freq = cpu_events.frequency;
  int f = 0;
  while (f < events_config.num_freqs && events_config.cpu_events_freq_config[f].frequency != freq) {
    f++;
  }
  if (f == events_config.num_freqs) {
    printf("%s:%d: CPU frequency %u not found in events_config.\n", __FILE__, __LINE__, freq);
    exit(1);
  }
  // set events
  for (int c = 0; c < NUM_CORES_CPU; c++) {
    for (int e = 0; e < NUM_COUNTERS_CPU; e++) {
      cpu_events.core[c].event[e] = events_config.cpu_events_freq_config[f].core[c].event[e];
    }
  }
  // de-init events_config
  free_events_config(&events_config);
  print_cpu_events();
}

void parse_cpu_events_json(char *config_file, cpu_events_config_t *events_config){
  jsmn_parser p;
  jsmntok_t t[1500]; // max 1500 tokens expected
  int ret;

  // read config_file into a string
  FILE *fp = fopen(config_file, "rb");
  if (fp == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, config_file);
    exit(1);
  }
  fseek(fp, 0L, SEEK_END);
  size_t sz = ftell(fp);
  rewind(fp);
  char *str_json = (char *)malloc(sz * sizeof(char));
  if (str_json == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  rewind(fp);
  fread(str_json, sz, 1, fp);
  fclose(fp);

  // parse json string into tokens
  jsmn_init(&p);
  ret = jsmn_parse(&p, str_json, sz, t, 1500);
  if (ret < 0) {
      printf("%s:%d: failed to parse JSON '%s' (error %d).\n", __FILE__, __LINE__, config_file, ret);
      exit(1);
  }

  uint32_t frequency;
  cpu_event_t event;
  unsigned int num_frequencies_json;
  unsigned int num_cores_json;
  unsigned int num_events_core_json;

  int i = 0;
  num_frequencies_json = t[i].size;
  // allocate events_config with num_frequencies_json frequencies
  events_config->num_freqs = num_frequencies_json;
  events_config->cpu_events_freq_config = (cpu_events_freq_config_t *)malloc(num_frequencies_json * sizeof(cpu_events_freq_config_t));
  if (events_config->cpu_events_freq_config == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }

  // loop over all keys of the root object
  for (int f = 0; f < num_frequencies_json; f++) {
    // frequency
    if (t[++i].type != JSMN_STRING) {
      printf("%s:%d: unexpected token type %d (expected JSMN_STRING).\n", __FILE__, __LINE__, t[i].type);
      exit(1);
    }
    jsmn_parse_token(str_json, &t[i], "%d", &frequency);
    events_config->cpu_events_freq_config[f].frequency = frequency;
    if (t[++i].type != JSMN_OBJECT) {
      printf("%s:%d: unexpected token type %d (expected JSMN_OBJECT).\n", __FILE__, __LINE__, t[i].type);
      exit(1);
    }
    num_cores_json = t[i].size;
    if (num_cores_json != NUM_CORES_CPU) {
      printf("%s:%d: unexpected number of cores %d (expected %d).\n", __FILE__, __LINE__, num_cores_json, NUM_CORES_CPU);
      exit(1);
    }
    // allocate space for cores
    events_config->cpu_events_freq_config[f].num_cores = num_cores_json;
    events_config->cpu_events_freq_config[f].core = (cpu_core_events_t *)malloc(num_cores_json * sizeof(cpu_core_events_t));
    if (events_config->cpu_events_freq_config[f].core == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    for (int c = 0; c < num_cores_json; c++) {
      // cores list
      if (t[++i].type != JSMN_STRING) {
        printf("%s:%d: unexpected token type %d (expected JSMN_STRING).\n", __FILE__, __LINE__, t[i].type);
        exit(1);
      }
      if (t[++i].type != JSMN_ARRAY) {
        printf("%s:%d: unexpected token type %d (expected JSMN_ARRAY).\n", __FILE__, __LINE__, t[i].type);
        exit(1);
      }
      num_events_core_json = t[i].size;
      // allocate space for events
      events_config->cpu_events_freq_config[f].core[c].num_events_core = num_events_core_json;
      events_config->cpu_events_freq_config[f].core[c].event = (cpu_event_t *)malloc(num_events_core_json * sizeof(cpu_event_t));
      if (events_config->cpu_events_freq_config[f].core[c].event == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      if (num_events_core_json != NUM_COUNTERS_CPU) {
        printf("%s:%d: unexpected number of events %d per core(expected %d).\n", __FILE__, __LINE__, num_events_core_json, NUM_COUNTERS_CPU);
        exit(1);
      }
      for (int e = 0; e < num_events_core_json; e++){
        // events list
        if (t[++i].type != JSMN_PRIMITIVE) {
          printf("%s:%d: unexpected token type %d (expected JSMN_PRIMITIVE).\n", __FILE__, __LINE__, t[i].type);
          exit(1);
        }
        jsmn_parse_token(str_json, &t[i], "%d", &event);
        events_config->cpu_events_freq_config[f].core[c].event[e] = event;
      }
    }
  }
}

uint32_t get_cpu_freq(){
#ifdef __JETSON_AGX_XAVIER
  uint32_t freq = 0;
  FILE *fp = fopen(CUR_FREQ_CPU_FILE, "r");
  if (fp == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, CUR_FREQ_CPU_FILE);
    exit(1);
  }
  fscanf(fp, "%d", &freq);
  fclose(fp);
  // fetched in kHz, convert to Hz
  return freq * 1000;
#else
#error "Platform not supported."
#endif
}

// clip CPU frequency to the closest available advertised frequency
uint32_t clip_cpu_freq(uint32_t freq){
#ifdef __JETSON_AGX_XAVIER
  // read a list of integers from a file
  uint32_t *avail_freqs = NULL;
  uint32_t num_avail_freqs = 0;
  uint32_t freq_read;
  FILE *fp = fopen(AVAIL_FREQ_CPU_FILE, "r");
  if (fp == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, AVAIL_FREQ_CPU_FILE);
    exit(1);
  }
  while (fscanf(fp, "%d", &freq_read) != EOF) {
    avail_freqs = (uint32_t *)realloc(avail_freqs, (num_avail_freqs + 1) * sizeof(uint32_t));
    if (avail_freqs == NULL) {
      printf("%s:%d: realloc failed.\n", __FILE__, __LINE__);
      exit(1);
    }
    // fetched in kHz, convert to Hz
    avail_freqs[num_avail_freqs] = freq_read * 1000;
    num_avail_freqs++;
  }
  fclose(fp);
  // find the closest available frequency
  uint32_t closest_freq = avail_freqs[0];
  for (int i = 1; i < num_avail_freqs; i++) {
    if (abs((int)freq - (int)avail_freqs[i]) < abs((int)freq - (int)closest_freq)) {
      closest_freq = avail_freqs[i];
    }
  }
  free(avail_freqs);
  // fetched in kHz, convert to Hz
  return closest_freq;
#else
#error "Platform not supported."
#endif
}

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                   Static functions                    ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static void print_cpu_events(){
  printf("Profiling CPU events:\n");
  for(int c = 0; c < cpu_events.num_cores; c++){
    printf("  [core %d] ", c);
    for(int e = 0; e < cpu_events.core[c].num_events_core; e++){
      printf("0x%02x ", cpu_events.core[c].event[e]);
    }
    printf("\n");
  }
}

static void free_events_config(cpu_events_config_t *events_config) {
  for (int f = 0; f < events_config->num_freqs; f++) {
    for (int c = 0; c < events_config->cpu_events_freq_config[f].num_cores; c++) {
      free(events_config->cpu_events_freq_config[f].core[c].event);
    }
    free(events_config->cpu_events_freq_config[f].core);
  }
  free(events_config->cpu_events_freq_config);
}
