# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include ./config/config.mk

all: voltmeter

# run voltmeter (run with sudo)
run: voltmeter kernelmod
	@mkdir -p $(TRACE_DIR)
	@cd $(INSTALL_DIR); \
	while read benchmark; do \
		echo $(INSTALL_DIR)/voltmeter $(VOLTMETER_ARGS) $$benchmark; \
		$(INSTALL_DIR)/voltmeter $(VOLTMETER_ARGS) $$benchmark || exit 1; \
	done <<< $(BENCHMARKS)

# compile voltmeter (run without sudo)
voltmeter: $(VOLTMETER_MK)
	mkdir -p $(INSTALL_DIR)
	$(MAKE) -C $(SRC_DIR) all

# parse config
$(VOLTMETER_MK): $(VOLTMETER_YML) $(UTILS_DIR)/parse_config/parse_config.py $(UTILS_DIR)/parse_config/yml_schema.py
	VOLTMETER_YML=$< VOLTMETER_MK=$@ $(UTILS_DIR)/parse_config/parse_config.py

# install kernel module for Carmel CPU counters profiling (NVIDIA Jetson)
kernelmod:
	sudo $(MAKE) -C $(UTILS_DIR)/carmel-module install

.PHONY: clean clean_traces

clean:
	sudo $(MAKE) -C $(SRC_DIR) clean
	sudo $(MAKE) -C $(UTILS_DIR)/carmel-module clean
	$(RM) -r $(INSTALL_DIR)
	$(RM) $(VOLTMETER_MK)

clean_traces:
	sudo $(RM) -r $(TRACE_DIR)
