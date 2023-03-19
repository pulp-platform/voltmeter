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
$(VOLTMETER_MK): $(VOLTMETER_YML) $(SCRIPTS_DIR)/parse_config.py
	VOLTMETER_YML=$< VOLTMETER_MK=$@ $(SCRIPTS_DIR)/parse_config.py

.PHONY: clean

clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(RM) $(VOLTMETER_MK)
	$(RM) -r $(TRACE_DIR)
