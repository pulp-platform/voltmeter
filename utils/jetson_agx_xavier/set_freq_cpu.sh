#!/usr/bin/env bash

# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

# $1 can be:
# - one of the frequencies (in kHz) from `sudo cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies`
# - 'min', to pick the min frequency from that list
# - 'max', to pick the max frequency from that list

echo "userspace" | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_governor > /dev/null

freqs=(`sudo cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies`)
if [ "$1" = "min" ]; then
  set_freq=${freqs[0]}
elif [ "$1" = "max" ]; then
  set_freq=${freqs[-1]}
else
  set_freq=$1
fi

echo "Setting CPU frequency to $set_freq kHz..."
echo $set_freq | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq > /dev/null
echo $set_freq | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq > /dev/null
echo $set_freq | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq > /dev/null
echo $set_freq | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed > /dev/null
