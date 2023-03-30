# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

SHELL = /bin/bash

# directories
ROOT_DIR    := $(abspath $(shell git rev-parse --show-toplevel 2>/dev/null))
CONFIG_DIR  := $(ROOT_DIR)/config
# files
VOLTMETER_YML := $(ROOT_DIR)/Voltmeter.yml
VOLTMETER_MK  := $(CONFIG_DIR)/voltmeter.mk

-include $(VOLTMETER_MK)

# more directories
SRC_DIR     := $(ROOT_DIR)/src
UTILS_DIR   := $(ROOT_DIR)/utils
INSTALL_DIR := $(ROOT_DIR)/install
TRACE_DIR   ?= $(trace_dir)

# extension of files to lint with clang-format
#TODO: Implement check (with clang lint or editorconfig checker)
CLANG_FORMAT_EXT := c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx,cu
