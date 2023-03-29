#!/usr/bin/env bash

# $1 is one of the frequencies (in kHz) from
# `sudo cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies`
echo "userspace" | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_governor > /dev/null
echo $1 | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq > /dev/null
echo $1 | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq > /dev/null
echo $1 | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed > /dev/null
