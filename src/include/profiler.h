// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _PROFILER_H
#define _PROFILER_H

// standard includes
#include <stdio.h>

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Macros                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// number of runs of each benchmark, to be averaged
#ifndef NUM_RUN
#define NUM_RUN 3
#endif
// sampling period duration in microseconds
#ifndef SAMPLE_PERIOD_US
#define SAMPLE_PERIOD_US 100000
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                         Types                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// arguments for thread call
typedef struct profiler_args {
  unsigned int thread_id;
  FILE *trace_file;
  volatile int *signal;
  pthread_barrier_t *barrier;
  unsigned int set_id_cpu;
  unsigned int set_id_gpu;
} profiler_args_t;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                     Declarations                      ║
 * ╚═══════════════════════════════════════════════════════╝
 */

void *events_profiler(void *args);

#endif // _PROFILER_H
