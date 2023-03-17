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

// Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <argp.h>
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
 * ║                        Macros                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */


// Parameters
#define TRACE_NAME_LEN 150

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
    {"return", 'r', "RETURN_MODE", 0, "Decide whether to run a profiling with the given events or just count the number of required passes; RETURN_MODE can be 'profile' or 'num_passes'", 5},
    {"trace_dir", 't', "TRACE_DIR", 0, "Path to the directory where to store the trace files; only if return == 'profile'", 6},
    {"benchmark_name", 'b', "BENCHMARK_NAME", 0, "Name of the benchmark to be profiled; only if return == 'profile'", 7},
    {0}
};

struct arguments {
  enum {NO_EVENTS, ALL_EVENTS, CONFIG, CLI} event_source;
#if CPU
  char *config_cpu;
  cpu_event_t *cli_cpu;
  unsigned int num_cli_cpu;
#endif
#if GPU
  char *config_gpu;
  gpu_event_t *cli_gpu;
  unsigned int num_cli_gpu;
#endif
  enum {NO_RETURN, PROFILE, NUM_PASSES} return_mode;
  char *trace_dir;
  char *benchmark_name;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state);

static struct argp argp = {options, parse_opt, args_doc, doc};


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
  arguments.return_mode = NO_RETURN;
  arguments.trace_dir = NULL;
  arguments.benchmark_name = NULL;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // print all arguments parsed by argp
  printf("\n═════════════════════════════════════\n");
  printf("              Voltmeter              \n");
  printf("─────────────────────────────────────\n");
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
  if (arguments.return_mode == PROFILE)
    printf("return_mode: profile\n");
  else if (arguments.return_mode == NUM_PASSES)
    printf("return_mode: num_passes\n");
  if (arguments.return_mode == PROFILE){
    printf("trace_dir: %s\n", arguments.trace_dir);
    printf("benchmark_name: %s\n", arguments.benchmark_name);
  }
  printf("═════════════════════════════════════\n\n");

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                     Setup devices                     │
 * └───────────────────────────────────────────────────────┘
*/

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
 * │                      Setup trace                      │
 * └───────────────────────────────────────────────────────┘
 */
  FILE *trace_file;

  // generate output trace path
  if (arguments.return_mode == PROFILE){
    // generate trace name
    char trace_name[TRACE_NAME_LEN] = {'\0'};
    sprintf(trace_name, "%s", arguments.benchmark_name);
#if CPU
    sprintf(trace_name + strlen(trace_name), "_cpu_%u", cpu_freq);
#endif
#if GPU
    sprintf(trace_name + strlen(trace_name), "_gpu_%u", gpu_freq);
#endif
    sprintf(trace_name + strlen(trace_name), ".bin");
    // allocate memory for trace path
    char *trace_path = malloc(strlen(arguments.trace_dir) + strlen(trace_name) + 10);
    if (trace_path == NULL){
      printf("%s:%d: failed to allocate memory.\n", __FILE__, __LINE__);
      exit(1);
    }
    // join trace_dir and trace_name
    cat_path(arguments.trace_dir, trace_name, trace_path);
    printf("Trace path: %s\n", trace_path);
    // open trace file
    trace_file = fopen(trace_path, "wb");
    if (trace_file == NULL){
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, trace_path);
      exit(1);
    }
  }

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                     Setup events                      │
 * └───────────────────────────────────────────────────────┘
 */
  int num_pass_gpu; // number of benchmark reaplays required to profile desired events

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

  if (arguments.return_mode == NUM_PASSES){
#if CPU
    printf("%s:%d: 'num_passes' return mode is not supported for CPU.\n", __FILE__, __LINE__);
    exit(1);
#endif
#if GPU
    return num_pass_gpu;
#endif
  }

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                        De-init                        │
 * └───────────────────────────────────────────────────────┘
 */
  // free memory
#if CPU
  free(arguments.cli_cpu);
  deinit_cpu();
#endif
#if GPU
  free(arguments.cli_gpu);
#endif
  // close files
  if (arguments.return_mode == PROFILE)
    fclose(trace_file);
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
      arguments->cli_cpu = (cpu_event_t*)malloc(sizeof(cpu_event_t));
      arguments->num_cli_cpu = 0;
      token = strtok(arg, ",");
      while (token != NULL){
        arguments->cli_cpu[arguments->num_cli_cpu] = atoi(token);
        arguments->num_cli_cpu++;
        arguments->cli_cpu = (cpu_event_t*)realloc(arguments->cli_cpu, (arguments->num_cli_cpu+1)*sizeof(cpu_event_t));
        token = strtok(NULL, ",");
      }
      break;
#endif
#if GPU
    case 'g':
      arguments->config_gpu = arg;
      break;
    case 'm':
      arguments->cli_gpu = (gpu_event_t*)malloc(sizeof(gpu_event_t));
      arguments->num_cli_gpu = 0;
      token = strtok(arg, ",");
      while (token != NULL){
        arguments->cli_gpu[arguments->num_cli_gpu] = atoi(token);
        arguments->num_cli_gpu++;
        arguments->cli_gpu = (gpu_event_t*)realloc(arguments->cli_gpu, (arguments->num_cli_gpu+1)*sizeof(gpu_event_t));
        token = strtok(NULL, ",");
      }
      break;
#endif
    case 'r':
      if (!strcmp(arg, "profile")){
        arguments->return_mode = PROFILE;
      } else if (!strcmp(arg, "num_passes")) {
        arguments->return_mode = NUM_PASSES;
      } else {
        argp_failure(state, 1, 0, "invalid argument for option %c: %s. See --help for more information.", key, arg);
      }
      break;
    case 't':
      arguments->trace_dir = arg;
      break;
    case 'b':
      arguments->benchmark_name = arg;
      break;
    case ARGP_KEY_END:
      // check event_source argument
      if (arguments->event_source == NO_EVENTS)
        argp_failure(state, 1, 0, "missing required argument for option --events. See --help for more information.");
      if (arguments->event_source == ALL_EVENTS)
        if (CPU)
          argp_failure(state, 1, 0, "--events all_events is not supported by CPU. See --help for more information.");
      if (arguments->event_source == CONFIG){
        if (CPU)
          if (arguments->config_cpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --config_cpu. See --help for more information.");
        if (GPU)
          if (arguments->config_gpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --config_gpu. See --help for more information.");
      }
      if (arguments->event_source == CLI){
        if (CPU)
          if (arguments->cli_cpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option --cli_cpu. See --help for more information.");
        if (GPU)
          if (arguments->cli_gpu == NULL)
            argp_failure(state, 1, 0, "missing required argument for option -cli_gpu. See --help for more information.");
      }
      // check return_mode argument
      if (arguments->return_mode == NO_RETURN)
        argp_failure(state, 1, 0, "missing required argument for option --return. See --help for more information.");
      if (arguments->return_mode == PROFILE){
        if (arguments->trace_dir == NULL)
          argp_failure(state, 1, 0, "missing required argument for option --trace_dir. See --help for more information.");
        if (arguments->benchmark_name == NULL)
          argp_failure(state, 1, 0, "missing required argument for option --benchmark_name. See --help for more information.");
      }
      if (arguments->return_mode == NUM_PASSES){
        if (CPU)
          argp_failure(state, 1, 0, "--return profile is not supported by CPU. See --help for more information.");
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
