# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

SHELL = /bin/bash

# directories
ROOT_DIR    := $(abspath $(shell git rev-parse --show-toplevel 2>/dev/null))
SRC_DIR     := $(ROOT_DIR)/src
SCRIPTS_DIR := $(ROOT_DIR)/scripts
CONFIG_DIR  := $(ROOT_DIR)/config
INSTALL_DIR := $(ROOT_DIR)/install
TRACE_DIR   ?= $(ROOT_DIR)/trace

# files
VOLTMETER_YML := $(CONFIG_DIR)/voltmeter.yml
VOLTMETER_MK  := $(CONFIG_DIR)/voltmeter.mk

-include $(VOLTMETER_MK)

# extension of files to lint with clang-format
CLANG_FORMAT_EXT := c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx,cu
