#!/usr/bin/env bash

# $1 is one of the frequencies (in Hz) from
# `sudo cat /sys/devices/17000000.gv11b/devfreq/17000000.gv11b/available_frequencies`
echo $1 | sudo tee /sys/devices/17000000.gv11b/devfreq/17000000.gv11b/max_freq > /dev/null
echo $1 | sudo tee /sys/devices/17000000.gv11b/devfreq/17000000.gv11b/min_freq > /dev/null
echo $1 | sudo tee /sys/devices/17000000.gv11b/devfreq/17000000.gv11b/max_freq > /dev/null
