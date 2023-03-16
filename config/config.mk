# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich

# directories
ROOT_DIR    := $(shell git rev-parse --show-toplevel 2>/dev/null)
SRC_DIR     := $(ROOT_DIR)/src
SCRIPTS_DIR := $(ROOT_DIR)/scripts
DOCS_DIR    := $(ROOT_DIR)/docs
CONFIG_DIR  := $(ROOT_DIR)/config
INSTALL_DIR := $(ROOT_DIR)/install
TRACE_DIR   ?= $(ROOT_DIR)/trace

# extension of files to lint with clang-format
CLANG_FORMAT_EXT := c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx,cu

####################
# Voltmeter macros #
####################

platform = jetson_agx_xavier

# compilation-time selection of what Voltmeter is going to profile
profile_cpu = 0
profile_gpu = 1

# number of runs of each benchmark to average
num_run = 3
# frame duration in microseconds
sample_period_us = 100000
