// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

// standard includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
// voltmeter libraries
#include <platform.h>
/*
 * ╔═══════════════════════════════════════════════════════╗
 * ║                        Globals                        ║
 * ╚═══════════════════════════════════════════════════════╝
 */

platform_power_t platform_power;

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

uint32_t setup_platform() {
#ifdef __JETSON_AGX_XAVIER
  platform_power.num_power_rails = NUM_POWER_RAILS;
#else
#error "Platform not supported."
#endif
}

void deinit_platform(){
#ifdef __JETSON_AGX_XAVIER
  // nothing to do
#else
#error "Platform not supported."
#endif
}

/*
 * ┌───────────────────────────────────────────────────────┐
 * │                  Power sensor driver                  │
 * └───────────────────────────────────────────────────────┘
 */

void read_platform_power() {
#ifdef __JETSON_AGX_XAVIER
  // Jetson built-in INA3221 power monitors (milliwatts, mW)
  FILE *ina;
  // I2C address 0x40, channel 0: GPU
  ina = fopen(INA_0x40_POWER_CH0_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[0]);
  fclose(ina);
  // I2C address 0x40, channel 1: CPU
  ina = fopen(INA_0x40_POWER_CH1_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[1]);
  fclose(ina);
  // I2C address 0x40, channel 2: SOC
  ina = fopen(INA_0x40_POWER_CH2_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[2]);
  fclose(ina);
  // I2C address 0x40, channel 0: CV
  ina = fopen(INA_0x41_POWER_CH0_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[3]);
  fclose(ina);
  // I2C address 0x40, channel 1: VDDRQ
  ina = fopen(INA_0x41_POWER_CH1_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[4]);
  fclose(ina);
  // I2C address 0x40, channel 2: SYS5V
  ina = fopen(INA_0x41_POWER_CH2_FILE, "r");
  if (ina == NULL) {
    printf("%s:%d: failed to open file '%s'.\n", __FILE__, __LINE__, INA_0x40_POWER_CH0_FILE);
    exit(1);
  }
  fscanf(ina, "%u", &platform_power.power_measures[5]);
  fclose(ina);
#else
#error "Platform not supported."
#endif
}
