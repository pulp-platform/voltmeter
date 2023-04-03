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
	$(SCRIPTS_DIR)/set_power_max.sh; \
	frequencies_cpu=($(frequencies_cpu)); \
	frequencies_gpu=($(frequencies_gpu)); \
	if [ -z "$$frequencies_cpu" ]; then \
		frequencies_cpu=(max); \
	fi; \
	if [ -z "$$frequencies_gpu" ]; then \
		frequencies_gpu=(max); \
	fi; \
	for freq_cpu in $${frequencies_cpu[@]}; do \
		$(SCRIPTS_DIR)/set_freq_cpu.sh $$freq_cpu; \
		for freq_gpu in $${frequencies_gpu[@]}; do \
			$(SCRIPTS_DIR)/set_freq_gpu.sh $$freq_gpu; \
			jetson_clocks --show; \
			while read benchmark; do \
				echo $(INSTALL_DIR)/voltmeter $(voltmeter_args) $$benchmark; \
				$(INSTALL_DIR)/voltmeter $(voltmeter_args) $$benchmark || exit 1; \
			done <<< $(benchmarks); \
		done; \
	done

# compile voltmeter
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
