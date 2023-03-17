// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

#ifndef _PLATFORM_H
#define _PLATFORM_H

// !!!!!!!!!!!!!
//  TEMPORARY
// !!!!!!!!!!!!!
//TODO: Remove this
#define __JETSON_AGX_XAVIER
#define CPU 1
#define GPU 1
//TODO: Remove this
// !!!!!!!!!!!!!
//  TEMPORARY
// !!!!!!!!!!!!!

#ifdef __JETSON_AGX_XAVIER
  // files
  #define INA_0x40_POWER_CH0_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power0_input"
  #define INA_0x40_POWER_CH1_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power1_input"
  #define INA_0x40_POWER_CH2_FILE "/sys/bus/i2c/drivers/ina3221x/1-0040/iio:device0/in_power2_input"
  #define INA_0x41_POWER_CH0_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device1/in_power0_input"
  #define INA_0x41_POWER_CH1_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device1/in_power1_input"
  #define INA_0x41_POWER_CH2_FILE "/sys/bus/i2c/drivers/ina3221x/1-0041/iio:device0/in_power2_input"
#else
  #error "Platform not supported."
#endif

#endif // _PLATFORM_H
