// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _PLATFORM_H
#define _PLATFORM_H

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Macros                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

// platform
#if !defined(__JETSON_AGX_XAVIER)
#error "No platform supported."
#endif
#ifndef CPU
#define CPU 0
#endif
#ifndef GPU
#define CPU 0
#endif

#ifdef __JETSON_AGX_XAVIER
  #define NUM_POWER_RAILS 6
  // files
  #define INA_0x40_POWER_CH0_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power0_input"
  #define INA_0x40_POWER_CH1_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power1_input"
  #define INA_0x40_POWER_CH2_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power2_input"
  #define INA_0x41_POWER_CH0_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device1/in_power0_input"
  #define INA_0x41_POWER_CH1_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device1/in_power1_input"
  #define INA_0x41_POWER_CH2_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device1/in_power2_input"
#else
  #error "Platform not supported."
#endif

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                         Types                         ║
 * ╚═══════════════════════════════════════════════════════╝
 */

typedef uint32_t power_t;

typedef struct {
  unsigned int num_power_rails;
  power_t power_measures[NUM_POWER_RAILS];
} platform_power_t;

/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                     Declarations                      ║
 * ╚═══════════════════════════════════════════════════════╝
 */

uint32_t setup_platform();
void deinit_platform();

void read_platform_power();

#endif // _PLATFORM_H
