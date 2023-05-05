# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include ./config/config.mk

.PHONY: clean clean_traces $(VOLTMETER_BIN)

all: $(VOLTMETER_BIN)

# run voltmeter (run with sudo)
run: $(VOLTMETER_BIN) $(VOLTMETER_MK) kernelmod
	@mkdir -p $(TRACE_DIR)
	@cd $(INSTALL_DIR); \
	$(PLATFORM_DIR)/set_power_max.sh; \
	frequencies_cpu=($(frequencies_cpu)); \
	frequencies_gpu=($(frequencies_gpu)); \
	if [ -z "$$frequencies_cpu" ]; then \
		frequencies_cpu=(max); \
	fi; \
	if [ -z "$$frequencies_gpu" ]; then \
		frequencies_gpu=(max); \
	fi; \
	for freq_cpu in $${frequencies_cpu[@]}; do \
		$(PLATFORM_DIR)/set_freq_cpu.sh $$freq_cpu; \
		for freq_gpu in $${frequencies_gpu[@]}; do \
			$(PLATFORM_DIR)/set_freq_gpu.sh $$freq_gpu; \
			jetson_clocks --show; \
			while read benchmark; do \
				echo $(VOLTMETER_BIN) $(voltmeter_args) $$benchmark; \
				$(VOLTMETER_BIN) $(voltmeter_args) $$benchmark || exit 1; \
			done <<< $(benchmarks); \
		done; \
	done; \
	echo "Profiling terminated without errors"

# compile voltmeter
$(VOLTMETER_BIN): $(VOLTMETER_MK)
	mkdir -p $(INSTALL_DIR)
	$(MAKE) -C $(SRC_DIR) all

# parse config
$(VOLTMETER_MK): $(VOLTMETER_YML) $(UTILS_DIR)/parse_config/parse_config.py $(UTILS_DIR)/parse_config/yml_schema.py
	VOLTMETER_YML=$< VOLTMETER_MK=$@ $(UTILS_DIR)/parse_config/parse_config.py

# install kernel module for Carmel CPU counters profiling (NVIDIA Jetson)
kernelmod: $(VOLTMETER_MK)
	if [ "$(platform)" = "jetson_agx_xavier" ]; then \
		sudo $(MAKE) -C $(PLATFORM_DIR)/carmel-module install; \
	fi

clean:
	sudo $(MAKE) -C $(SRC_DIR) clean
	sudo $(MAKE) -C $(PLATFORM_DIR)/carmel-module clean
	sudo $(RM) -r $(INSTALL_DIR)
	$(RM) $(VOLTMETER_MK)

clean_traces:
	sudo $(RM) -r $(TRACE_DIR)
	sudo $(RM) -r $(ROOT_DIR)/traces*
