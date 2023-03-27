// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// third-party libraries
#include <jsmn.h>
#ifdef __JETSON_AGX_XAVIER
#include <cuda_runtime_api.h>
#include <cupti_events.h>
#endif
// voltmeter libraries
#include <helper.h>
#include <gpu.h>

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                      Prototypes                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static void print_gpu_events();
static void free_events_freq_config(gpu_events_freq_config_t *events_freq_config);
static void free_events_config(gpu_events_config_t *events_config);
#ifdef __JETSON_AGX_XAVIER
static uint32_t cupti_create_event_group_sets(CUpti_EventID *event_ids, int num_events_tot);
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Globals                        ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static gpu_events_freq_config_t gpu_events;

#ifdef __JETSON_AGX_XAVIER
static CUdevice cu_device;
static CUcontext cu_context;
#endif

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

uint32_t setup_gpu(){
#ifdef __JETSON_AGX_XAVIER
  CUresult ret_cuda;
  int cuda_device_count;
  char cuda_device_name[50];

  // verify CUDA functionalities and drivers functioning
  ret_cuda = cuInit(0);
  CHECK_CU_ERROR(ret_cuda, "cuInit");
  ret_cuda = cuDeviceGetCount(&cuda_device_count);
  CHECK_CU_ERROR(ret_cuda, "cuDeviceGetCount");
  if (cuda_device_count == 0) {
      printf("%s:%d: no device supporting CUDA detected.\n", __FILE__, __LINE__);
      exit(1);
  }
  // statically select device CUDA_DEV_NUM
  printf("CUDA device number: %d\n", CUDA_DEV_NUM);
  ret_cuda = cuDeviceGet(&cu_device, CUDA_DEV_NUM);
  CHECK_CU_ERROR(ret_cuda, "cuDeviceGet");
  ret_cuda = cuDeviceGetName(cuda_device_name, 50, cu_device);
  CHECK_CU_ERROR(ret_cuda, "cuDeviceGetName");
  printf("CUDA device name: %s\n", cuda_device_name);
  // create context for this GPU
  ret_cuda = cuCtxCreate(&cu_context, 0, cu_device);
  CHECK_CU_ERROR(ret_cuda, "cuCtxCreate");

  // fill global variable gpu_events
  gpu_events.frequency = clip_gpu_freq(get_gpu_freq());
  // will be allocated in gpu_events_all/gpu_events_from_config/gpu_events_from_cli
  gpu_events.num_counters = 0;
  gpu_events.event_id = NULL;
  gpu_events.event_group_sets = NULL;
  gpu_events.counter = NULL;

  return gpu_events.frequency;
#else
#error "Platform not supported."
#endif
}

void deinit_gpu(){
  free_events_freq_config(&gpu_events);
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                    Events parsing                     │
 * └───────────────────────────────────────────────────────┘
 */

uint32_t gpu_events_all(){
#ifdef __JETSON_AGX_XAVIER
  CUptiResult ret;
  size_t size;

  // get number of device event domains
  uint32_t num_domains;
  ret = cuptiDeviceGetNumEventDomains(cu_device, &num_domains);
  CHECK_CUPTI_ERROR(ret, "cuptiDeviceGetNumEventDomains");
  if (num_domains == 0) {
    printf("%s:%d: no CUPTI event domains found.\n", __FILE__, __LINE__);
    exit(1);
  }
  // allocate space for array of all domain IDs
  size = sizeof(CUpti_EventDomainID) * num_domains;
  CUpti_EventDomainID *domain_ids = (CUpti_EventDomainID *)malloc(size);
  if (domain_ids == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  memset(domain_ids, 0, size);
  // enumerate all domain IDs
  ret = cuptiDeviceEnumEventDomains(cu_device, &size, domain_ids);
  CHECK_CUPTI_ERROR(ret, "cuptiDeviceEnumEventDomains");

  // get number of events in each domain + accumulate it
  uint32_t *num_domain_events;
  uint32_t num_events_tot = 0;
  num_domain_events = (uint32_t *)malloc(num_domains * sizeof(uint32_t));
  if (num_domain_events == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  for (int i = 0; i < num_domains; i++) {
    ret = cuptiEventDomainGetNumEvents(domain_ids[i], &num_domain_events[i]);
    CHECK_CUPTI_ERROR(ret, "cuptiEventDomainGetNumEvents");
    num_events_tot += num_domain_events[i];
  }

  // allocate buffer for output
  CUpti_EventID *event_ids;
  event_ids = (CUpti_EventID *)malloc(num_events_tot * sizeof(CUpti_EventID));
  if (event_ids == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  // concatenate event IDs from all domains into event_ids
  CUpti_EventID *event_array_ptr = event_ids;
  for (int d = 0; d < num_domains; d++) {
    size = sizeof(CUpti_EventID) * num_domain_events[d];
    ret = cuptiEventDomainEnumEvents(domain_ids[d], &size, event_array_ptr);
    CHECK_CUPTI_ERROR(ret, "cuptiEventDomainEnumEvents");
    event_array_ptr += num_domain_events[d];
  }

  printf("Total number of exposed GPU events: %u\n", num_events_tot);
  // create CUPTI event group sets (i.e., sets of CUPTI event groups)
  uint32_t num_sets = cupti_create_event_group_sets(event_ids, num_events_tot);
  gpu_events.event_id = event_ids;
  gpu_events.num_counters = num_events_tot;
  // de-init
  free(domain_ids);
  free(num_domain_events);
  return num_sets;
#else
#error "Platform not supported."
#endif
}

uint32_t gpu_events_from_cli(gpu_event_id_t *events, unsigned int num_events){
#ifdef __JETSON_AGX_XAVIER
  gpu_events.num_counters = num_events;
  gpu_events.event_id = (gpu_event_id_t *)malloc(num_events * sizeof(gpu_event_id_t));
  if (gpu_events.event_id == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  for (int i = 0; i < num_events; i++){
    gpu_events.event_id[i] = events[i];
  }
  // create CUPTI event group sets (i.e., sets of CUPTI event groups)
  print_gpu_events();
  uint32_t num_sets = cupti_create_event_group_sets(events, num_events);
  return num_sets;
#else
#error "Platform not supported."
#endif
}

uint32_t gpu_events_from_config(char *config_file){
  gpu_events_config_t events_config;
  parse_gpu_events_json(config_file, &events_config);

  // find GPU frequency in events_config
  uint32_t freq = gpu_events.frequency;
  int f = 0;
  while (f < events_config.num_freqs && events_config.gpu_events_freq_config[f].frequency != freq) {
    f++;
  }
  if (f == events_config.num_freqs) {
    printf("%s:%d: GPU frequency %u not found in events_config.\n", __FILE__, __LINE__, freq);
    exit(1);
  }
  // set events
  gpu_events.num_counters = events_config.gpu_events_freq_config[f].num_counters;
  gpu_events.event_id = (gpu_event_id_t *)malloc(gpu_events.num_counters * sizeof(gpu_event_id_t));
  for (int e = 0; e < events_config.gpu_events_freq_config[f].num_counters; e++) {
    gpu_events.event_id[e] = events_config.gpu_events_freq_config[f].event_id[e];
  }
  // create CUPTI event group sets (i.e., sets of CUPTI event groups)
  print_gpu_events();
  uint32_t num_sets = cupti_create_event_group_sets(events_config.gpu_events_freq_config[f].event_id, events_config.gpu_events_freq_config[f].num_counters);
  // free events_config
  free_events_config(&events_config);
  return num_sets;
}

void parse_gpu_events_json(char *config_file, gpu_events_config_t *events_config){
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
  ret = jsmn_parse(&p, str_json, strlen(str_json), t, 1500);
  if (ret < 0) {
      printf("%s:%d: failed to parse JSON '%s' (error %d).\n", __FILE__, __LINE__, config_file, ret);
      exit(1);
  }

  uint32_t frequency;
  gpu_event_id_t event;
  unsigned int num_frequencies_json;
  unsigned int num_events_json;

  int i = 0;
  num_frequencies_json = t[i].size;
  // allocate events_config with num_frequencies_json frequencies
  events_config->num_freqs = num_frequencies_json;
  events_config->gpu_events_freq_config = (gpu_events_freq_config_t *)malloc(num_frequencies_json * sizeof(gpu_events_freq_config_t));
  if (events_config->gpu_events_freq_config == NULL) {
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
    //printf("%.*s\n", t[i].end - t[i].start, str_json + t[i].start); // DEBUG
    jsmn_parse_token(str_json, &t[i], "%d", &frequency);
    events_config->gpu_events_freq_config[f].frequency = frequency;
    if (t[++i].type != JSMN_ARRAY) {
      printf("%s:%d: unexpected token type %d (expected JSMN_ARRAY).\n", __FILE__, __LINE__, t[i].type);
      exit(1);
    }
    //printf("%.*s\n", t[i].end - t[i].start, str_json + t[i].start); // DEBUG
    num_events_json = t[i].size;
    // allocate space for events at this frequency
    events_config->gpu_events_freq_config[f].counter = NULL; // to prevent from Segmentation fault when freeing unused field
    events_config->gpu_events_freq_config[f].num_counters = num_events_json;
    events_config->gpu_events_freq_config[f].event_id = (gpu_event_id_t *)malloc(num_events_json * sizeof(gpu_event_id_t));
    if (events_config->gpu_events_freq_config[f].event_id == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    for (int e = 0; e < num_events_json; e++){
      // events list
      if (t[++i].type != JSMN_PRIMITIVE) {
        printf("%s:%d: unexpected token type %d (expected JSMN_PRIMITIVE).\n", __FILE__, __LINE__, t[i].type);
        exit(1);
      }
      //printf("%.*s\n", t[i].end - t[i].start, str_json + t[i].start); // DEBUG
      jsmn_parse_token(str_json, &t[i], "%d", &event);
      events_config->gpu_events_freq_config[f].event_id[e] = event;
    }
  }
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                      PMU driver                       │
 * └───────────────────────────────────────────────────────┘
 */

void enable_pmu_gpu(unsigned int set_id) {
#ifdef __JETSON_AGX_XAVIER
  CUpti_EventGroupSet group_set = gpu_events.event_group_sets->sets[set_id];
  CUptiResult ret;

  // set continuous event collection mode for context
  ret = cuptiSetEventCollectionMode(cu_context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS);
  CHECK_CUPTI_ERROR(ret, "cuptiSetEventCollectionMode");
  // mallocs
  gpu_events.num_events_group = (uint32_t *)malloc(group_set.numEventGroups * sizeof(uint32_t));
  if (gpu_events.num_events_group == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  gpu_events.num_instances_group = (uint32_t *)malloc(group_set.numEventGroups * sizeof(uint32_t));
  if (gpu_events.num_instances_group == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  gpu_events.sizes_event_ids_group = (size_t *)malloc(group_set.numEventGroups * sizeof(size_t));
  if (gpu_events.sizes_event_ids_group == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  gpu_events.sizes_counters_group = (size_t *)malloc(group_set.numEventGroups * sizeof(size_t));
  if (gpu_events.sizes_counters_group == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  gpu_events.event_ids_buffer = (CUpti_EventID **)malloc(group_set.numEventGroups * sizeof(CUpti_EventID*));
  if (gpu_events.event_ids_buffer == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }
  gpu_events.counters_buffer = (gpu_counter_t **)malloc(group_set.numEventGroups * sizeof(gpu_counter_t*));
  if (gpu_events.counters_buffer == NULL) {
    printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
    exit(1);
  }

  // for each event group in the set
  for (int g = 0; g < group_set.numEventGroups; g++){
    CUpti_EventGroup group = group_set.eventGroups[g];
    unsigned int one = 1; // required for cuptiEventGroupSetAttribute
    size_t size;

    // set to profile all domain instances
    ret = cuptiEventGroupSetAttribute(group, CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES, sizeof(one), &one);
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupSetAttribute");

    // get events number in each group
    size = sizeof(gpu_events.num_events_group[g]);
    ret = cuptiEventGroupGetAttribute(group, CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS, &size, &gpu_events.num_events_group[g]);
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupGetAttribute");
    // get instances number in each group
    size = sizeof(gpu_events.num_instances_group[g]);
    ret = cuptiEventGroupGetAttribute(group, CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT, &size, &gpu_events.num_instances_group[g]);
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupGetAttribute");

    // for each group, calculate the size of the array to hold all the event ids in the group
    gpu_events.sizes_event_ids_group[g] = sizeof(CUpti_EventID) * gpu_events.num_events_group[g];
    // for each group, calculate the size of the array to hold all the counter values for all instances in the group
    gpu_events.sizes_counters_group[g] = sizeof(gpu_counter_t) * gpu_events.num_events_group[g] * gpu_events.num_instances_group[g];

    // allocate memory for the event ids and counter values buffers
    gpu_events.event_ids_buffer[g] = (CUpti_EventID *)malloc(gpu_events.sizes_event_ids_group[g]);
    if (gpu_events.event_ids_buffer[g] == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    gpu_events.counters_buffer[g] = (gpu_counter_t *)malloc(gpu_events.sizes_counters_group[g]);
    if (gpu_events.counters_buffer[g] == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }

    // enable counting
    ret = cuptiEventGroupEnable(group);
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupEnable");
  }
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void disable_pmu_gpu(unsigned int set_id) {
#ifdef __JETSON_AGX_XAVIER
  CUptiResult ret;
  gpu_events.event_group_sets->sets[set_id];
  for (int g = 0; g < gpu_events.event_group_sets->sets[set_id].numEventGroups; g++){
    ret = cuptiEventGroupDisable(gpu_events.event_group_sets->sets[set_id].eventGroups[g]);
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupDisable");
  }

  free(gpu_events.num_events_group);
  free(gpu_events.num_instances_group);
  free(gpu_events.sizes_event_ids_group);
  free(gpu_events.sizes_counters_group);
  for (int g = 0; g < gpu_events.event_group_sets->sets[set_id].numEventGroups; g++){
    free(gpu_events.event_ids_buffer[g]);
    free(gpu_events.counters_buffer[g]);
  }
  free(gpu_events.event_ids_buffer);
  free(gpu_events.counters_buffer);
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void read_counters_gpu(unsigned int set_id) {
#ifdef __JETSON_AGX_XAVIER
  CUptiResult ret;
  CUpti_EventGroupSet group_set = gpu_events.event_group_sets->sets[set_id];
  for (int g = 0; g < group_set.numEventGroups; g++){
    size_t num_event_ids;
    CUpti_EventGroup group = group_set.eventGroups[g];
    // read events from group g and automatically reset counters
    num_event_ids = 0;
    ret = cuptiEventGroupReadAllEvents(
      group, CUPTI_EVENT_READ_FLAG_NONE,
      &gpu_events.sizes_counters_group[g], gpu_events.counters_buffer[g],
      &gpu_events.sizes_event_ids_group[g], gpu_events.event_ids_buffer[g],
      &num_event_ids
    );
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupReadAllEvents");
    if (num_event_ids != gpu_events.num_events_group[g]) {
      printf("%s:%d: number of read GPU events (%d) does not match number of events in group (%d).\n", __FILE__, __LINE__, num_event_ids, gpu_events.num_events_group[g]);
      exit(1);
    }
  }
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

void reset_counters_gpu() {
#ifdef __JETSON_AGX_XAVIER
  printf("%s:%d: reset_counters_gpu not implemented.\n", __FILE__, __LINE__);
  exit(1);
#else
#error "Unsupported platform/architecture/compiler".
#endif
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                   Helper functions                    │
 * └───────────────────────────────────────────────────────┘
 */

uint32_t get_gpu_freq(){
#ifdef __JETSON_AGX_XAVIER
  uint32_t freq = 0;
  FILE *fp = fopen(CUR_FREQ_GPU_FILE, "r");
  if (fp == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, CUR_FREQ_GPU_FILE);
    exit(1);
  }
  fscanf(fp, "%d", &freq);
  fclose(fp);
  // fetched in Hz
  return freq;
#else
#error "Platform not supported."
#endif
}

// Clip GPU frequency to the closest available advertised frequency
uint32_t clip_gpu_freq(uint32_t freq){
#ifdef __JETSON_AGX_XAVIER
  // read a list of integers from a file
  uint32_t *avail_freqs = NULL;
  uint32_t num_avail_freqs = 0;
  uint32_t freq_read;
  FILE *fp = fopen(AVAIL_FREQ_GPU_FILE, "r");
  if (fp == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, AVAIL_FREQ_GPU_FILE);
    exit(1);
  }
  while (fscanf(fp, "%d", &freq_read) != EOF) {
    avail_freqs = (uint32_t *)realloc(avail_freqs, (num_avail_freqs + 1) * sizeof(uint32_t));
    if (avail_freqs == NULL) {
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    // fetched in kHz, convert to Hz
    avail_freqs[num_avail_freqs] = freq_read;
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

static void print_gpu_events(){
  printf("Profiling GPU events:\n ");
  for (int e = 0; e < gpu_events.num_counters; e++) {
    printf("%u ", gpu_events.event_id[e]);
  }
  printf("\n");
}

static void free_events_freq_config(gpu_events_freq_config_t *events_freq_config){
  free(events_freq_config->event_id);
#ifdef __JETSON_AGX_XAVIER
  CUptiResult ret;
  ret = cuptiEventGroupSetsDestroy(events_freq_config->event_group_sets);
  if (ret != CUPTI_ERROR_INVALID_PARAMETER)  // to prevent from Segmentation fault when freeing unused field
    CHECK_CUPTI_ERROR(ret, "cuptiEventGroupSetsDestroy");
#endif
  free(events_freq_config->counter);
}

static void free_events_config(gpu_events_config_t *events_config){
  for (int f = 0; f < events_config->num_freqs; f++) {
    free_events_freq_config(&events_config->gpu_events_freq_config[f]);
  }
  free(events_config->gpu_events_freq_config);
}


#ifdef __JETSON_AGX_XAVIER
// CUPTI-specific functions

static uint32_t cupti_create_event_group_sets(CUpti_EventID *event_ids, int num_events_tot){
  CUptiResult ret;
  size_t size;
  size = sizeof(CUpti_EventID) * num_events_tot;
  ret = cuptiEventGroupSetsCreate(cu_context, size, event_ids, &gpu_events.event_group_sets);
  CHECK_CUPTI_ERROR(ret, "cuptiEventGroupSetsCreate");
  // return number of requires passes to profile all contained events
  printf("Number of CUPTI event group sets (required passes): %u\n", gpu_events.event_group_sets->numSets);
  return gpu_events.event_group_sets->numSets;
}

#endif
