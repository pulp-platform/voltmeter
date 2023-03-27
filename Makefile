# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include ./config/config.mk

all: voltmeter

# run voltmeter
run: voltmeter
	@while read benchmark; do \
		echo $(INSTALL_DIR)/voltmeter $(VOLTMETER_ARGS) $$benchmark; \
		$(INSTALL_DIR)/voltmeter $(VOLTMETER_ARGS) $$benchmark; \
	done <<< $(BENCHMARKS)

# compile voltmeter
voltmeter: $(VOLTMETER_MK)
	mkdir -p $(INSTALL_DIR)
	$(MAKE) -C $(SRC_DIR) all

# parse config
$(VOLTMETER_MK): $(VOLTMETER_YML) $(SCRIPTS_DIR)/parse_config.py $(SCRIPTS_DIR)/yml_schema.py
	VOLTMETER_YML=$< VOLTMETER_MK=$@ $(SCRIPTS_DIR)/parse_config.py

.PHONY: clean

clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(RM) $(VOLTMETER_MK)
	$(RM) -r $(TRACE_DIR)
