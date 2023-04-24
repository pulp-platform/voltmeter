# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

SHELL = /bin/bash

# directories
CONFIG_DIR := $(abspath $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
ROOT_DIR   := $(abspath $(CONFIG_DIR)/..)
PATCH_DIR  := $(ROOT_DIR)/patch

RODINIA_DIR := $(ROOT_DIR)/rodinia
