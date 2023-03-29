// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// standard includes
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
// voltmeter libraries
#include <profiler.h>
#include <platform.h>
#if CPU
#include <cpu.h>
#endif
#if GPU
#include <gpu.h>
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Extern                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

#if CPU
extern cpu_events_freq_config_t cpu_events;
#endif
#if GPU
extern gpu_events_freq_config_t gpu_events;
#endif
extern platform_power_t platform_power;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                       Functions                       ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// events profiler function, to be called through phtread
void *events_profiler(void *args) {
  profiler_args_t *thread_args = (profiler_args_t*)args;
  struct timespec timestamp_a, timestamp_b;
  uint64_t sampling_time;

#if CPU
  // enable CPU PMU
  enable_pmu_cpu_core(thread_args->thread_id);
#endif
#if GPU
  // enable GPU PMU (only CPU thread 0 is the GPU host)
  if (thread_args->thread_id == 0)
    enable_pmu_gpu(thread_args->set_id_gpu);
#endif

  // write trace header (only CPU thread 0 opens the trace file)
  if (thread_args->thread_id == 0) {
#if CPU
    fwrite(&cpu_events.num_cores, sizeof(uint32_t), 1, thread_args->trace_file);
    for (int c = 0; c < cpu_events.num_cores; c++) {
      fwrite(&cpu_events.core[c].num_counters_core, sizeof(uint32_t), 1, thread_args->trace_file);
      fwrite(cpu_events.core[c].event_id, sizeof(cpu_event_id_t) * cpu_events.core[c].num_counters_core, 1, thread_args->trace_file);
    }
#endif
#if GPU
#ifdef __JETSON_AGX_XAVIER
    fwrite(&gpu_events.event_group_sets->sets[thread_args->set_id_gpu].numEventGroups, sizeof(uint32_t), 1, thread_args->trace_file);
    for (int g = 0; g < gpu_events.event_group_sets->sets[thread_args->set_id_gpu].numEventGroups; g++) {
      fwrite(&gpu_events.num_events_group[g], sizeof(uint32_t), 1, thread_args->trace_file);
      fwrite(&gpu_events.num_instances_group[g], sizeof(uint32_t), 1, thread_args->trace_file);
      fwrite(gpu_events.event_ids_buffer[g], sizeof(gpu_event_id_t), gpu_events.num_events_group[g], thread_args->trace_file);
    }
#else
#error "Platform not supported."
#endif
#endif
    fwrite(&platform_power.num_power_rails, sizeof(uint32_t), 1, thread_args->trace_file);
  }

  // wait for all cores
  pthread_barrier_wait(thread_args->barrier);

  //////////////////////////////////
  // start profiler sampling period
  //////////////////////////////////

  while(!(*thread_args->signal)) {
    // start overhead measurement
    if (thread_args->thread_id == 0)
      clock_gettime(CLOCK_REALTIME, &timestamp_a);

#if CPU
    // sample CPU counters
    read_counters_cpu_core(thread_args->thread_id);
    reset_counters_cpu_core();
    // sample current CPU frequency
    read_cpu_core_freq(thread_args->thread_id);
#endif
#if GPU
    if (thread_args->thread_id == 0) {
      // sample GPU counters (reset on read)
      read_counters_gpu(thread_args->set_id_gpu);
      // sample current GPU frequency
      read_gpu_freq();
    }
#endif

    // fetch power measures
    if (thread_args->thread_id == 0)
      read_platform_power();

    // wait for all cores to have sampled CPU counters
    pthread_barrier_wait(thread_args->barrier);

    // write trace: GPU freq, GPU ids+counters, power
    if (thread_args->thread_id == 0) {
#if CPU
      // per each core: CPU freq, CPU counter values
      for(int c = 0; c < cpu_events.num_cores; c++) {
        fwrite(&cpu_events.core[c].freq_read, sizeof(uint32_t), 1, thread_args->trace_file);
        fwrite(cpu_events.core[c].counter, sizeof(cpu_counter_t), cpu_events.core[c].num_counters_core, thread_args->trace_file);
#ifdef __JETSON_AGX_XAVIER
        fwrite(&cpu_events.core[c].counter_clk, sizeof(uint64_t), 1, thread_args->trace_file);
#endif
      }
#endif
#if GPU
#ifdef __JETSON_AGX_XAVIER
      // GPU freq, per each group: per each instance: GPU counter values
      fwrite(&gpu_events.freq_read, sizeof(uint32_t), 1, thread_args->trace_file);
      for (int g = 0; g < gpu_events.event_group_sets->sets[thread_args->set_id_gpu].numEventGroups; g++)
        fwrite(gpu_events.counters_buffer[g], sizeof(gpu_counter_t), gpu_events.num_events_group[g] * gpu_events.num_instances_group[g], thread_args->trace_file);
#else
#error "Platform not supported."
#endif
#endif
      // power measures
      fwrite(platform_power.power_measures, sizeof(power_t), platform_power.num_power_rails, thread_args->trace_file);
    }

    // sync before measuring time
    pthread_barrier_wait(thread_args->barrier);

    if (thread_args->thread_id == 0) {
      // stop overhead measurement
      clock_gettime(CLOCK_REALTIME, &timestamp_b);
      sampling_time = (timestamp_b.tv_sec - timestamp_a.tv_sec) * 1e9 + (timestamp_b.tv_nsec - timestamp_a.tv_nsec);
      // dump overhead measurement to trace file
      fwrite(&sampling_time, sizeof(uint64_t), 1, thread_args->trace_file); // nanoseconds, ns
      // count remaining time to sampling period and sleep
      if (sampling_time / 1000 < SAMPLE_PERIOD_US)
        usleep(SAMPLE_PERIOD_US - sampling_time / 1000);
    }

    // sync before new iteration
    pthread_barrier_wait(thread_args->barrier);
  }

#if CPU
  // de-init CPU PMU
  disable_pmu_cpu_core();
#endif
#if GPU
  if (thread_args->thread_id == 0) {
    // de-init GPU PMU
    disable_pmu_gpu();
  }
#endif
}
