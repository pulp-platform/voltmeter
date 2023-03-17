# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include ../config/config.mk

TARGET := voltmeter
TARGET := $(addprefix $(INSTALL_DIR)/,$(TARGET))

BUILD_DIR ?= $(SRC_DIR)/build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -type f)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# includes
INC_DIRS := $(shell find $(SRC_DIR) -path $(BUILD_DIR) -prune -o -type d -print)
INCLUDES += $(addprefix -I,$(INC_DIRS))
# additional includes
INCLUDES += -I$(CUDA_PATH)/../include -I$(CUDA_PATH)/include

# libraries
LIB_DIRS := $(CUDA_PATH)/../lib64 $(CUDA_PATH)/lib64
LIBS += $(addprefix -L,$(LIB_DIRS))
# additional libraries
LIBS += -ldl -lcuda -lcudart -lm -lcupti

# defines
DEFINES += -DCPU=$(PROFILE_CPU) -DGPU=$(PROFILE_GPU)
DEFINES += -DNUM_RUN=$(NUM_RUN) -DSAMPLE_PERIOD_US=$(SAMPLE_PERIOD_US)
ifeq ($(PLATFORM),jetson_agx_xavier)
DEFINES += -D__JETSON_AGX_XAVIER
endif

# additional flags
CFLAGS ?= $(LIBS) $(INCLUDES) -MMD -MP $(DEFINES)

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(CFLAGS)

# C source
$(BUILD_DIR)/%.o: %.c $(CONFIG_DIR)/config.mk $(VOLTMETER_YML) $(VOLTMETER_MK)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) $(TARGET)

-include $(DEPS)
