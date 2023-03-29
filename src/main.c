// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// Supported devices
#ifndef CPU
#define CPU 0
#endif
#ifndef GPU
#define GPU 0
#endif

#if !CPU && !GPU
#error No supported devices have been specified.
#endif

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <argp.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sched.h>
// voltmeter libraries
#include <platform.h>
#include <profiler.h>
#include <helper.h>
#if CPU
#include <cpu.h>
#endif
#if GPU
#include <gpu.h>
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                  Argp configuration                   ║
 * ╚═══════════════════════════════════════════════════════╝
 */

const char *argp_program_version = "voltmeter 1.0";
const char *argp_program_bug_address = "<smazzola@iis.ee.ethz.ch>";
static char doc[] = "Power and performance counters profiler.";
static char args_doc[] = "";
static struct argp_option options[] = {
    {"events", 'e', "EVENTS_SOURCE", 0, "Source of events to profile; EVENTS_SOURCE can be 'all_events', 'config', or 'cli'", 0},
#if CPU
    {"config_cpu", 'c', "CONFIG_FILE_CPU", 0, "Path to the event configuration file for the CPU profiling; only if events == 'config'", 1},
    {"cli_cpu", 'l', "CLI_EVENTS_CPU", 0, "List of CPU events to profile, separated by commas; only if events == 'cli'", 3},
#endif
#if GPU
    {"config_gpu", 'g', "CONFIG_FILE_GPU", 0, "Path to the event configuration file for the GPU profiling; only if events == 'config'", 2},
    {"cli_gpu", 'm', "CLI_EVENTS_GPU", 0, "List of GPU events to profile, separated by commas; only if events == 'cli'", 4},
#endif
    {"mode", 'r', "MODE", 0, "Decide in which mode to run Voltmeter; MODE can be 'char', 'profile' 'num_passes'", 5},
    {"trace_dir", 't', "TRACE_DIR", 0, "Path to the directory where to store the trace files; only if mode == 'char' or 'profile'", 6},
    {"benchmark", 'b', "BENCHMARK_PATH", 0, "Path of benchmark compiled as a dynamic library; only if mode == 'char' or 'profile'", 7},
    {"benchmark_args", 'a', "BENCHMARK_ARGS", 0, "Comma-separated arguments to be passed to the benchmark, in the same order; only if mode == 'char' or 'profile'", 8},
    {0}
};

struct arguments {
  enum {NO_EVENTS, ALL_EVENTS, CONFIG, CLI} event_source;
#if CPU
  char *config_cpu;
  cpu_event_id_t *cli_cpu;
  unsigned int num_cli_cpu;
#endif
#if GPU
  char *config_gpu;
  gpu_event_id_t *cli_gpu;
  unsigned int num_cli_gpu;
#endif
  enum {NO_MODE, CHARACTERIZATION, PROFILE, NUM_PASSES} mode;
  char *trace_dir;
  char *benchmark;
  char **benchmark_args;
  unsigned int num_benchmark_args;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state);

static struct argp argp = {options, parse_opt, args_doc, doc};

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Extern                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

extern cpu_events_freq_config_t cpu_events;
/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                         Main                          ║
 * ╚═══════════════════════════════════════════════════════╝
 */

int main(int argc, char *argv[]) {

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                    Parse arguments                    │
 * └───────────────────────────────────────────────────────┘
 */

  struct arguments arguments;
  arguments.event_source = NO_EVENTS;
#if CPU
  arguments.config_cpu = NULL;
  arguments.cli_cpu = NULL;
  arguments.num_cli_cpu = 0;
#endif
#if GPU
  arguments.config_gpu = NULL;
  arguments.cli_gpu = NULL;
  arguments.num_cli_gpu = 0;
#endif
  arguments.mode = NO_MODE;
  arguments.trace_dir = NULL;
  arguments.benchmark = NULL;
  arguments.benchmark_args = NULL;
  arguments.num_benchmark_args = 0;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // print all arguments parsed by argp
  printf("\n════════════════════════════════════════════════════════════════════════════════\n");
  printf("                                   Voltmeter                                    \n");
  printf("────────────────────────────────────────────────────────────────────────────────\n");
  printf("Supported devices: ");
  if (CPU)
    printf("CPU ");
  if (GPU)
    printf("GPU");
  printf("\n");
  if (arguments.event_source == ALL_EVENTS)
    printf("event_source: all_events\n");
  else if (arguments.event_source == CONFIG)
    printf("event_source: config\n");
  else if (arguments.event_source == CLI)
    printf("event_source: cli\n");
  if (arguments.event_source == CONFIG) {
#if CPU
    printf("config_cpu: %s\n", arguments.config_cpu);
#endif
#if GPU
    printf("config_gpu: %s\n", arguments.config_gpu);
#endif
  } else if (arguments.event_source == CLI) {
#if CPU
    printf("cli_cpu: ");
    for (int i = 0; i < arguments.num_cli_cpu; i++)
      printf("%u ", arguments.cli_cpu[i]);
    printf("\n");
#endif
#if GPU
    printf("cli_gpu: ");
    for (int i = 0; i < arguments.num_cli_gpu; i++)
      printf("%u ", arguments.cli_gpu[i]);
    printf("\n");
#endif
  }
  if (arguments.mode == CHARACTERIZATION)
    printf("mode: characterization\n");
  else if (arguments.mode == PROFILE)
    printf("mode: profile\n");
  else if (arguments.mode == NUM_PASSES)
    printf("mode: num_passes\n");
  if (arguments.mode == PROFILE){
    printf("trace_dir: %s\n", arguments.trace_dir);
    printf("benchmark: %s\n", arguments.benchmark);
    printf("benchmark_args: ");
    for (int i = 1; i <= arguments.num_benchmark_args; i++)
      printf("%s ", arguments.benchmark_args[i]);
    printf("\n");
  }
  printf("════════════════════════════════════════════════════════════════════════════════\n\n");

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                     Setup devices                     │
 * └───────────────────────────────────────────────────────┘
*/

  setup_platform();
#if CPU
  uint32_t cpu_freq = setup_cpu();
  printf("Current CPU frequency: %u Hz\n", cpu_freq);
#endif
#if GPU
  uint32_t gpu_freq = setup_gpu();
  printf("Current GPU frequency: %u Hz\n", gpu_freq);
#endif

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                     Setup events                      │
 * └───────────────────────────────────────────────────────┘
 */

  // GPU events profiling may require multiple serial passes
  int num_pass_gpu = 1; // number of benchmark reaplays required to profile desired events

  // event_source: all_events
  if (arguments.event_source == ALL_EVENTS) {
#if CPU
    printf("%s:%d: 'all_events' event source is not supported for CPU.\n", __FILE__, __LINE__);
    exit(1);
#endif
#if GPU
    num_pass_gpu = gpu_events_all();
#endif
  }
  // event_source: config
  else if (arguments.event_source == CONFIG) {
#if CPU
    cpu_events_from_config(arguments.config_cpu);
#endif
#if GPU
    num_pass_gpu = gpu_events_from_config(arguments.config_gpu);
#endif
  }
  // event_source: cli
  else if (arguments.event_source == CLI) {
#if CPU
    cpu_events_from_cli(arguments.cli_cpu, arguments.num_cli_cpu);
#endif
#if GPU
    num_pass_gpu = gpu_events_from_cli(arguments.cli_gpu, arguments.num_cli_gpu);
#endif
  }

  if (arguments.mode == NUM_PASSES){
#if CPU
    printf("%s:%d: 'num_passes' mode is not supported for CPU.\n", __FILE__, __LINE__);
    exit(1);
#endif
#if GPU
    return num_pass_gpu;
#endif
  }

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                       Profiling                       │
 * └───────────────────────────────────────────────────────┘
 */

  if (arguments.mode == CHARACTERIZATION || arguments.mode == PROFILE){

    // strip path from arguments.benchmark
    char *benchmark_path = strdup(arguments.benchmark);
    char *benchmark_name = path_basename(arguments.benchmark);
    // prepare benchmark
    void* benchmark_handle = dlopen(benchmark_path, RTLD_NOW | RTLD_LOCAL);
    if (!benchmark_handle) {
        fputs(dlerror(), stdout);
        printf("\n");
        printf("%s:%d: benchmark %s cannot be opened.\n", __FILE__, __LINE__, benchmark_path);
        exit(1);
    }
    void (*benchmark)(int argc, char** argv) = dlsym(benchmark_handle, "main");
    if (!benchmark) {
        fputs(dlerror(), stdout);
        printf("\n");
        printf("%s:%d: benchmark %s cannot be run.\n", __FILE__, __LINE__, benchmark_path);
        exit(1);
    }

    if (num_pass_gpu > 1) {
      if (arguments.mode == PROFILE) {
        printf("%s:%d: 'profile' mode cannot have multiple passes.\n", __FILE__, __LINE__);
        exit(1);
      } else if (arguments.mode == CHARACTERIZATION) {
        if (CPU) {
          printf("%s:%d: 'characterization' mode does not support multiple passes for CPU; you can perform multiple passes to profile incompatible counters by launching Voltmeter multiple times.\n", __FILE__, __LINE__);
          exit(1);
        }
      }
    }

    //////////////////////////////////
    // set up profiler and benchmark
    //////////////////////////////////

    for (int p = 0; p < num_pass_gpu; p++) {
      printf("\n");
      // setup traces
      FILE *trace_file;
      char *trace_path = NULL;

      // generate trace name
      unsigned int trace_i = 0;
      char trace_name[150] = {'\0'};
      do {
        // this loop creates numbered traces if benchmarks with same name are profiled:
        // useful when same benchmark is profiled multiple times with different arguments,
        // or when multiple passes are performed with same configuration but different counters
        sprintf(trace_name, "%s", benchmark_name);
#if CPU
        sprintf(trace_name + strlen(trace_name), "_cpu_%u", cpu_freq);
#endif
#if GPU
        sprintf(trace_name + strlen(trace_name), "_gpu_%u", gpu_freq);
#endif
        if (trace_i)
          sprintf(trace_name + strlen(trace_name), "_%u", trace_i); // add suffix if != 0
        sprintf(trace_name + strlen(trace_name), ".bin");
        // allocate memory for trace path
        trace_path = realloc(trace_path, strlen(arguments.trace_dir) + strlen(trace_name) + 10);
        if (trace_path == NULL){
          printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
          exit(1);
        }
        // join trace_dir and trace_name
        cat_path(arguments.trace_dir, trace_name, trace_path);
        printf("Trace path: %s\n", trace_path);
        trace_i++;
      } while(access(trace_path, F_OK) != -1);
      // open trace file
      trace_file = fopen(trace_path, "wb");
      if (trace_file == NULL){
      printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, trace_path);
        exit(1);
      }

      // setup profiler
      profiler_args_t *profiler_args = NULL;
      size_t num_profiler_threads = 0;
      cpu_set_t cpu_set;
      pthread_t *profiler_threads;
      pthread_attr_t pthread_attr;
      pthread_barrier_t profiler_barrier;
      int ret = 0;
      volatile int benchmark_complete = 0;

      // allocate profiler_args
#if CPU
      num_profiler_threads = cpu_events.num_cores;
#else
      num_profiler_threads = 1;
#endif
      profiler_args = malloc(sizeof(profiler_args_t) * num_profiler_threads);
      if (profiler_args == NULL){
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      profiler_threads = malloc(sizeof(pthread_t) * num_profiler_threads);
      if (profiler_threads == NULL){
        printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
        exit(1);
      }
      // init profiler thread(s) barrier
      pthread_barrier_init(&profiler_barrier, NULL, num_profiler_threads);
      // launch profiler thread(s)
      printf("\n");
      for (int t = 0; t < num_profiler_threads; t++) {
        printf("Initializing profiler thread for core %d...\n", t);
        // setup profiler arguments
        profiler_args[t].thread_id = t;
        profiler_args[t].trace_file = trace_file;
        profiler_args[t].signal = &benchmark_complete;
        profiler_args[t].barrier = &profiler_barrier;
        profiler_args[t].set_id_gpu = p;
        // set up pthread
        pthread_attr_init(&pthread_attr);
        CPU_ZERO(&cpu_set);
        CPU_SET(t, &cpu_set);
        pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set);
        // create thread c limiting its affinity to only CPU c
        ret = pthread_create(&profiler_threads[t], &pthread_attr, events_profiler, &profiler_args[t]);
        if (ret != 0) {
          perror("pthread_create");
          printf("%s:%d: failed to create profiler thread.\n", __FILE__, __LINE__);
          exit(1);
        }
      }

      // run benchmark
      arguments.benchmark_args[0] = benchmark_path; // assign argv[0]
      printf("\n");
      printf("Running benchmark '%s' with %d argument(s).\n", benchmark_name, arguments.num_benchmark_args);
      printf("Benchmark arguments:\n");
      for (int i = 0; i <= arguments.num_benchmark_args + 1; i++)
        printf("  argv[%d] = %s \n", i, arguments.benchmark_args[i]);
        printf("\n");
        printf("────────────────────────────────────────────────────────────────────────────────\n");
      for (int r = 0; r < NUM_PASSES; r++) {
        printf(" Benchmark pass %d/%d\n", r + 1, NUM_PASSES);
        printf("────────────────────────────────────────────────────────────────────────────────\n");
        printf("\n");
        // benchmarks needs to return with 'return' and not 'exit'
        benchmark(arguments.num_benchmark_args + 1, arguments.benchmark_args);
        printf("\n");
        printf("────────────────────────────────────────────────────────────────────────────────\n");
      }
      printf("\n");
      // signal profiler threads to stop
      benchmark_complete = 1;
      // close benchmark
      dlclose(benchmark_handle);
      // handle dl errors
      char *error = dlerror();
      if (error != NULL) {
        fputs(error, stdout);
        printf("\n");
        printf("%s:%d: benchmark %s cannot be run.\n", __FILE__, __LINE__, benchmark_path);
        exit(1);
      }

      printf("Benchmark '%s' finished.\n\n", benchmark_name);
      // join profiler threads
      for (int t = 0; t < num_profiler_threads; t++) {
        ret = pthread_join(profiler_threads[t], NULL);
        if (ret != 0) {
          perror("pthread_join");
          printf("%s:%d: failed to join profiler thread.\n", __FILE__, __LINE__);
          exit(1);
        }
        printf("Profiler thread %d has ended.\n", t);
      }
      // free profiler_args
      free(profiler_args);
      free(profiler_threads);
      // clean traces variables
      fclose(trace_file);
      free(trace_path);
      // destroy profiler thread(s) barrier
      pthread_barrier_destroy(&profiler_barrier);

#if GPU
      // wait for the benchmark on GPU to finish (1 task running on GPU at a time)
      sync_gpu_slave();
#endif
    }
  }

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                        De-init                        │
 * └───────────────────────────────────────────────────────┘
 */

  deinit_platform();
  // free memory
#if CPU
  free(arguments.cli_cpu);
  deinit_cpu();
#endif
#if GPU
  free(arguments.cli_gpu);
#endif
  return 0;
}


/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                       Functions                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

static error_t parse_opt(int key, char *arg, struct argp_state *state){
  struct arguments *arguments = state->input;
  char *token;
  // parse arguments
  switch(key){
    case 'e':
      if (!strcmp(arg, "all_events")){
        arguments->event_source = ALL_EVENTS;
      } else if (!strcmp(arg, "config")) {
        arguments->event_source = CONFIG;
      } else if (!strcmp(arg, "cli")) {
        arguments->event_source = CLI;
      } else {
        argp_failure(state, 1, 0, "invalid argument for option %c: %s. See --help for more information.", key, arg);
      }
      break;
#if CPU
    case 'c':
      arguments->config_cpu = arg;
      break;
    case 'l':
      arguments->cli_cpu = (cpu_event_id_t*)malloc(sizeof(cpu_event_id_t));
      arguments->num_cli_cpu = 0;
      token = strtok(arg, ",");
      while (token != NULL){
        arguments->cli_cpu[arguments->num_cli_cpu] = atoi(token);
        arguments->num_cli_cpu++;
        arguments->cli_cpu = (cpu_event_id_t*)realloc(arguments->cli_cpu, (arguments->num_cli_cpu+1)*sizeof(cpu_event_id_t));
        if (arguments->cli_cpu == NULL){
          printf("%s:%d: realloc failed.\n", __FILE__, __LINE__);
          exit(1);
        }
        token = strtok(NULL, ",");
      }
      break;
#endif
#if GPU
    case 'g':
      arguments->config_gpu = arg;
      break;
    case 'm':
      arguments->cli_gpu = (gpu_event_id_t*)malloc(sizeof(gpu_event_id_t));
      arguments->num_cli_gpu = 0;
      token = strtok(arg, ",");
      while (token != NULL){
        arguments->cli_gpu[arguments->num_cli_gpu] = atoi(token);
        arguments->num_cli_gpu++;
        arguments->cli_gpu = (gpu_event_id_t*)realloc(arguments->cli_gpu, (arguments->num_cli_gpu+1)*sizeof(gpu_event_id_t));
        if (arguments->cli_gpu == NULL){
          printf("%s:%d: realloc failed.\n", __FILE__, __LINE__);
          exit(1);
        }
        token = strtok(NULL, ",");
      }
      break;
#endif
    case 'r':
      if (!strcmp(arg, "characterization")){
        arguments->mode = CHARACTERIZATION;
      } else if (!strcmp(arg, "profile")){
        arguments->mode = PROFILE;
      } else if (!strcmp(arg, "num_passes")) {
        arguments->mode = NUM_PASSES;
      } else {
        argp_failure(state, 1, 0, "invalid argument for option %c: %s. See --help for more information.", key, arg);
      }
      break;
    case 't':
      arguments->trace_dir = arg;
      break;
    case 'b':
      arguments->benchmark = arg;
      break;
    case 'a':
      // parse benchmark arguments into array of strings
      arguments->num_benchmark_args = 0;
      if (arg == NULL){
        // handle if benchmark has no arguments: for argv[0] and NULL
        arguments->benchmark_args = malloc(2 * sizeof(char*));
        if (arguments->benchmark_args == NULL){
          printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
          exit(1);
        }
      } else {
        char *token = strtok(arg, ",");
        while (token != NULL) {
          arguments->num_benchmark_args++;
          arguments->benchmark_args = realloc(arguments->benchmark_args, (arguments->num_benchmark_args + 2) * sizeof(char*));
          if (arguments->benchmark_args == NULL){
            printf("%s:%d: realloc failed.\n", __FILE__, __LINE__);
            exit(1);
          }
          arguments->benchmark_args[arguments->num_benchmark_args] = token;
          token = strtok(NULL, ",");
        }
      }
      arguments->benchmark_args[arguments->num_benchmark_args+1] = NULL; // argv[argc] should be NULL
      break;
    case ARGP_KEY_END:
      // check event_source argument
      if (arguments->event_source == NO_EVENTS)
        argp_failure(state, 1, 0, "missing required argument for option --events. See --help for more information.");
      if (arguments->event_source == ALL_EVENTS)
        if (CPU)
          argp_failure(state, 1, 0, "--events all_events is not supported by CPU. See --help for more information.");
      if (arguments->event_source == CONFIG){
#if CPU
          if (arguments->config_cpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --config_cpu. See --help for more information.");
#endif
#if GPU
          if (arguments->config_gpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --config_gpu. See --help for more information.");
#endif
      }
      if (arguments->event_source == CLI){
#if CPU
          if (arguments->cli_cpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --cli_cpu. See --help for more information.");
#endif
#if GPU
          if (arguments->cli_gpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option -cli_gpu. See --help for more information.");
#endif
      }
      // check mode argument
      if (arguments->mode == NO_MODE)
        argp_failure(state, 1, 0, "missing required argument for option --mode. See --help for more information.");
      if (arguments->mode == CHARACTERIZATION)
        if (CPU && GPU)
          argp_failure(state, 1, 0, "--mode characterization only supports one device at a time. See --help for more information.");
      if (arguments->mode == CHARACTERIZATION || arguments->mode == PROFILE){
        if (arguments->trace_dir == NULL)
          argp_failure(state, 1, 0, "missing required argument for option --trace_dir. See --help for more information.");
        if (arguments->benchmark == NULL)
          argp_failure(state, 1, 0, "missing required argument for option --benchmark. See --help for more information.");
      }
      if (arguments->mode == NUM_PASSES){
        if (CPU)
          argp_failure(state, 1, 0, "--mode num_passes is not supported by CPU. See --help for more information.");
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
