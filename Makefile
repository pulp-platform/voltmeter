include ./config/config.mk

# compile voltmeter
voltmeter: $(VOLTMETER_MK)
		mkdir -p $(INSTALL_DIR)
		$(MAKE) -C $(SRC_DIR) all

# parse config
$(VOLTMETER_MK): $(VOLTMETER_YML)
		VOLTMETER_YML=$^ VOLTMETER_MK=$@ $(SCRIPTS_DIR)/parse_config.py

.PHONY: clean

clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(RM) $(VOLTMETER_MK)
	$(RM) $(TRACE_DIR)
