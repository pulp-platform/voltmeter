// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>
//         Thomas Benz, ETH Zurich <tbenz@iis.ee.ethz.ch>
//         Björn Forsberg

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
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

static void free_events_freq_config(cpu_events_freq_config_t *events_freq_config);
static void free_events_config(cpu_events_config_t *events_config);

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Globals                        ║
 * ╚═══════════════════════════════════════════════════════╝
 */

cpu_events_freq_config_t cpu_events;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                       Functions                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                         Setup                         │
 * └───────────────────────────────────────────────────────┘
 */

uint32_t setup_cpu(FILE *log_file) {
#ifdef __JETSON_AGX_XAVIER
  // init global variable cpu_events
  cpu_events.frequency = clip_cpu_freq(get_cpu_freq());
  cpu_events.num_cores = NUM_CORES_CPU;
  cpu_events.core = (cpu_core_events_t *)malloc(NUM_CORES_CPU * sizeof(cpu_core_events_t));
  if (cpu_events.core == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  for (int i = 0; i < NUM_CORES_CPU; i++) {
    cpu_events.core[i].freq_read = 0;
    cpu_events.core[i].counter_clk = 0;
    cpu_events.core[i].counter_set = NULL;
    cpu_events.core[i].num_sets = 0;
  }
  printf_file(log_file, "CPU cores: %d\n", NUM_CORES_CPU);
  printf_file(log_file, "Counters per CPU core: %d\n", NUM_COUNTERS_CPU);
  return cpu_events.frequency;
#else
#error "Platform not supported."
#endif
}

void deinit_cpu(){
  free_events_freq_config(&cpu_events);
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                    Events parsing                     │
 * └───────────────────────────────────────────────────────┘
 */

unsigned int cpu_events_all(FILE *log_file) {
#ifdef __JETSON_AGX_XAVIER
  unsigned int max_id = 0xFF;
  // compute number of required sets
  cpu_events.core[0].num_sets = (unsigned int)ceil((double)(max_id + 1) / NUM_COUNTERS_CPU);
  // compute for cpu_events.core[0], then copy pointer to all cores
  cpu_events.core[0].counter_set = (cpu_counter_set_t*)malloc(sizeof(cpu_counter_set_t) * cpu_events.core[0].num_sets);
  if (cpu_events.core[0].counter_set == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  for (int s = 0; s < cpu_events.core[0].num_sets; s++) {
    cpu_events.core[0].counter_set[s].num_counters = NUM_COUNTERS_CPU;
    cpu_events.core[0].counter_set[s].counter = (cpu_counter_t*)malloc(sizeof(cpu_counter_t) * NUM_COUNTERS_CPU);
    if (cpu_events.core[0].counter_set[s].counter == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    cpu_events.core[0].counter_set[s].event_id = (cpu_event_id_t*)malloc(sizeof(cpu_event_id_t) * NUM_COUNTERS_CPU);
    if (cpu_events.core[0].counter_set[s].event_id == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    for (int e = 0; e < NUM_COUNTERS_CPU; e++) {
      cpu_events.core[0].counter_set[s].event_id[e] = (cpu_event_id_t)(s * NUM_COUNTERS_CPU + e);
    }
  }
  // copy pointer to all cores
  for (int c = 1; c < cpu_events.num_cores; c++) {
    cpu_events.core[c].counter_set = cpu_events.core[0].counter_set;
  }
  return cpu_events.core[0].num_sets;
#else
#error "Platform not supported."
#endif
}

unsigned int cpu_events_from_cli(cpu_event_id_t *events, unsigned int num_events, FILE *log_file){
#ifdef __JETSON_AGX_XAVIER
  // only supports cpu_events.core[c].num_sets = 1
  // events are expected in the order:
  // core0_event0 core0_event1 core0_event2 core1_event0 core1_event1 ...
  if (num_events != NUM_CORES_CPU * NUM_COUNTERS_CPU) {
    printf("%s:%d: unexpected number of events %d (expected %d).\n", __FILE__, __LINE__, num_events, NUM_CORES_CPU * NUM_COUNTERS_CPU);
    exit(1);
  }
  for (int c = 0; c < cpu_events.num_cores; c++) {
    cpu_events.core[c].num_sets = 1; // only 1 set supported (i.e. 3 counters per core)
    cpu_events.core[c].counter_set = (cpu_counter_set_t*)malloc(sizeof(cpu_counter_set_t) * cpu_events.core[c].num_sets);
    if (cpu_events.core[c].counter_set == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    for (int s = 0; s < cpu_events.core[c].num_sets; s++) {
      cpu_events.core[c].counter_set[s].num_counters = NUM_COUNTERS_CPU;
      cpu_events.core[c].counter_set[s].counter = (cpu_counter_t*)malloc(sizeof(cpu_counter_t) * NUM_COUNTERS_CPU);
      if (cpu_events.core[c].counter_set[s].counter == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      cpu_events.core[c].counter_set[s].event_id = (cpu_event_id_t*)malloc(sizeof(cpu_event_id_t) * NUM_COUNTERS_CPU);
      if (cpu_events.core[c].counter_set[s].event_id == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      for (int e = 0; e < NUM_COUNTERS_CPU; e++) {
        cpu_events.core[c].counter_set[s].event_id[e] = events[c * NUM_COUNTERS_CPU + e];
      }
    }
  }
  return cpu_events.core[0].num_sets; // num_sets is the same for all cores
#else
#error "Platform not supported."
#endif
}

unsigned int cpu_events_from_config(char *config_file, FILE *log_file) {
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
  for (int c = 0; c < cpu_events.num_cores; c++) {
    cpu_events.core[c].num_sets =  events_config.cpu_events_freq_config[f].core[c].num_sets;
    cpu_events.core[c].counter_set = (cpu_counter_set_t*)malloc(sizeof(cpu_counter_set_t) * cpu_events.core[c].num_sets);
    if (cpu_events.core[c].counter_set == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    for (int s = 0; s < cpu_events.core[c].num_sets; s++) {
      cpu_events.core[c].counter_set[s].num_counters = NUM_COUNTERS_CPU;
      cpu_events.core[c].counter_set[s].counter = (cpu_counter_t*)malloc(sizeof(cpu_counter_t) * NUM_COUNTERS_CPU);
      if (cpu_events.core[c].counter_set[s].counter == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      cpu_events.core[c].counter_set[s].event_id = (cpu_event_id_t*)malloc(sizeof(cpu_event_id_t) * NUM_COUNTERS_CPU);
      if (cpu_events.core[c].counter_set[s].event_id == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      for (int e = 0; e < NUM_COUNTERS_CPU; e++) {
        cpu_events.core[c].counter_set[s].event_id[e] = events_config.cpu_events_freq_config[f].core[c].counter_set[s].event_id[e];
      }
    }
  }
  // de-init events_config
  free_events_config(&events_config);
  return cpu_events.core[0].num_sets; // num_sets is the same for all cores
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
  cpu_event_id_t event;
  unsigned int num_frequencies_json;
  unsigned int num_cores_json;
  unsigned int num_counters_core_json;

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
      num_counters_core_json = t[i].size;
      if (num_counters_core_json != NUM_COUNTERS_CPU) {
        printf("%s:%d: unexpected number of events %d per core(expected %d).\n", __FILE__, __LINE__, num_counters_core_json, NUM_COUNTERS_CPU);
        exit(1);
      }

      // allocate space for events
      events_config->cpu_events_freq_config[f].core[c].num_sets = 1; // only 1 set supported (i.e. 3 counters per core)
      events_config->cpu_events_freq_config[f].core[c].counter_set = (cpu_counter_set_t *)malloc(events_config->cpu_events_freq_config[f].core[c].num_sets * sizeof(cpu_counter_set_t));
      if (events_config->cpu_events_freq_config[f].core[c].counter_set == NULL) {
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      for (int s = 0; s < events_config->cpu_events_freq_config[f].core[c].num_sets; s++) {
        events_config->cpu_events_freq_config[f].core[c].counter_set[s].num_counters = num_counters_core_json;
        events_config->cpu_events_freq_config[f].core[c].counter_set[s].counter = NULL; // to prevent from Segmentation fault when freeing unused field
        events_config->cpu_events_freq_config[f].core[c].counter_set[s].event_id = (cpu_event_id_t *)malloc(num_counters_core_json * sizeof(cpu_event_id_t));
        if (events_config->cpu_events_freq_config[f].core[c].counter_set[s].event_id == NULL) {
          printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
          exit(1);
        }
      }

      for (int e = 0; e < num_counters_core_json; e++){
        // events list
        if (t[++i].type != JSMN_PRIMITIVE) {
          printf("%s:%d: unexpected token type %d (expected JSMN_PRIMITIVE).\n", __FILE__, __LINE__, t[i].type);
          exit(1);
        }
        jsmn_parse_token(str_json, &t[i], "%d", &event);
        // assign events (only 1 set supported)
        events_config->cpu_events_freq_config[f].core[c].counter_set[0].event_id[e] = event;
      }
    }
  }
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                      PMU driver                       │
 * └───────────────────────────────────────────────────────┘
 */

void enable_pmu_cpu_core(unsigned int core_id, unsigned int set_id) {
#if defined(__GNUC__) && defined(__aarch64__) && defined(__JETSON_AGX_XAVIER)
  cpu_event_id_t event[NUM_COUNTERS_CPU];

  for (int i = 0; i < NUM_COUNTERS_CPU; i++)
    event[i] = cpu_events.core[core_id].counter_set[set_id].event_id[i] & ARMV8_PMEVTYPER_EVTCOUNT_MASK;

  __asm__ __volatile__("isb");
  // use counters 0-2
  __asm__ __volatile__("msr pmevtyper0_el0, %0" : : "r" (event[0]));
  __asm__ __volatile__("msr pmevtyper1_el0, %0" : : "r" (event[1]));
  __asm__ __volatile__("msr pmevtyper2_el0, %0" : : "r" (event[2]));

  // Performance Monitors Count Enable Set register bit 30:1 disable, 31,1 enable
  uint32_t r = 0;
  __asm__ __volatile__("mrs %0, pmcr_el0" : "=r" (r));
  // clear counters
  __asm__ __volatile__("msr pmcr_el0, %0" : : "r" (r|(1<<1)|(1<<2)));

  __asm__ __volatile__("mrs %0, pmcntenset_el0" : "=r" (r));
  __asm__ __volatile__("msr pmcntenset_el0, %0" : : "r" (r|111));
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void disable_pmu_cpu_core() {
#if defined(__GNUC__) && defined(__aarch64__) && defined(__JETSON_AGX_XAVIER)
  // Performance Monitors Count Enable Set register: clear bit 0
  uint32_t r = 0;

  __asm__ __volatile__("mrs %0, pmcntenset_el0" : "=r" (r));
  __asm__ __volatile__("msr pmcntenset_el0, %0" : : "r" (r && 0xfffffff8));
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void read_counters_cpu_core(unsigned int core_id, unsigned int set_id) {
#if defined(__GNUC__) && defined(__aarch64__) && defined(__JETSON_AGX_XAVIER)
  __asm__ __volatile__("mrs %0, pmccntr_el0"   : "=r" (cpu_events.core[core_id].counter_clk));
  __asm__ __volatile__("mrs %0, pmevcntr0_el0" : "=r" (cpu_events.core[core_id].counter_set[set_id].counter[0]));
  __asm__ __volatile__("mrs %0, pmevcntr1_el0" : "=r" (cpu_events.core[core_id].counter_set[set_id].counter[1]));
  __asm__ __volatile__("mrs %0, pmevcntr2_el0" : "=r" (cpu_events.core[core_id].counter_set[set_id].counter[2]));
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void reset_counters_cpu_core() {
#if defined(__GNUC__) && defined(__aarch64__) && defined(__JETSON_AGX_XAVIER)
  // Performance Monitors Count Enable Set register
  uint32_t r = 0;
  __asm__ __volatile__("mrs %0, pmcr_el0" : "=r" (r));
  // clear counters
  __asm__ __volatile__("msr pmcr_el0, %0" : : "r" (r|(1<<1)|(1<<2)));
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void read_cpu_core_freq(unsigned int core_id) {
  cpu_events.core[core_id].freq_read = get_cpu_core_freq(core_id);
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                   Helper functions                    │
 * └───────────────────────────────────────────────────────┘
 */

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

uint32_t get_cpu_core_freq(unsigned int core_id){
#ifdef __JETSON_AGX_XAVIER
  uint32_t freq = 0;
  char freq_core_file[100];
  sprintf(freq_core_file, CORE_FREQ_CPU_FILE, core_id);
  FILE *fp = fopen(freq_core_file, "r");
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

void print_cpu_events(FILE *log_file) {
  printf_file(log_file, "Profiling CPU events:\n");
  for(int c = 0; c < cpu_events.num_cores; c++){
    printf_file(log_file, "  [core %d] ", c);
    for (int s = 0; s < cpu_events.core[0].num_sets; s++) { // num_sets is the same for all cores
      printf_file(log_file, "(");
      for(int e = 0; e < cpu_events.core[c].counter_set[s].num_counters; e++){
        printf_file(log_file, "0x%02x", cpu_events.core[c].counter_set[s].event_id[e]);
        if (e != cpu_events.core[c].counter_set[s].num_counters - 1) {
          printf_file(log_file, ",");
        }
      }
      printf_file(log_file, ") ");
    }
    printf_file(log_file, "\n");
  }
}

void print_cpu_events_set(FILE *log_file, unsigned int set_id) {
  printf_file(log_file, "Profiling CPU events (set %u):\n", set_id);
  for(int c = 0; c < cpu_events.num_cores; c++){
    printf_file(log_file, "  [core %d] ", c);
    for(int e = 0; e < cpu_events.core[c].counter_set[set_id].num_counters; e++){
      printf_file(log_file, "0x%02x ", cpu_events.core[c].counter_set[set_id].event_id[e]);
    }
    printf_file(log_file, "\n");
  }
}

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                   Static functions                    ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static void free_events_freq_config(cpu_events_freq_config_t *events_freq_config) {
  for (int c = 0; c < events_freq_config->num_cores; c++) {
    for (int s = 0; s < events_freq_config->core[c].num_sets; s++) {
      free(events_freq_config->core[c].counter_set[s].event_id);
      free(events_freq_config->core[c].counter_set[s].counter);
    }
    //free(events_freq_config->core[c].counter_set);
  }
  free(events_freq_config->core);
}

static void free_events_config(cpu_events_config_t *events_config) {
  for (int f = 0; f < events_config->num_freqs; f++) {
    free_events_freq_config(&events_config->cpu_events_freq_config[f]);
  }
  free(events_config->cpu_events_freq_config);
}
