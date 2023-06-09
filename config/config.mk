# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

SHELL = /bin/bash

# directories
ROOT_DIR    := $(abspath $(shell git rev-parse --show-toplevel 2>/dev/null))
CONFIG_DIR  := $(ROOT_DIR)/config
SRC_DIR     := $(ROOT_DIR)/src
UTILS_DIR   := $(ROOT_DIR)/utils
INSTALL_DIR := $(ROOT_DIR)/install
# files
VOLTMETER_YML ?= $(ROOT_DIR)/Voltmeter.yml # default Manifest (can be changed!)
VOLTMETER     := $(shell echo "$(basename $(notdir $(VOLTMETER_YML)))" | tr A-Z a-z)
VOLTMETER_MK  := $(CONFIG_DIR)/$(VOLTMETER).mk
VOLTMETER_BIN := $(INSTALL_DIR)/$(VOLTMETER)

-include $(VOLTMETER_MK)

# more directories
TRACE_DIR ?= $(trace_dir)

# platform-specific
ifeq ($(platform),jetson_agx_xavier)
ifndef CUDA_PATH
CUDA_PATH := /usr/local/cuda
endif
PLATFORM_DIR := $(UTILS_DIR)/jetson_agx_xavier
endif

# extension of files to lint with clang-format
