# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include ../config/config.mk

# target
TARGET := voltmeter
TARGET := $(addprefix $(INSTALL_DIR)/,$(TARGET))
BUILD_DIR ?= $(SRC_DIR)/build

# source
SRCS := $(shell find $(SRC_DIR) -name "*.c" -type f)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# general includes
INC_DIRS += $(shell find $(SRC_DIR) -path $(BUILD_DIR) -prune -o -type d -print)
INC_DIRS += $(includes)
# general libraries
LIB_DIRS += $(libraries)
LIBS     += -ldl -lm
# general flags
FLAGS    += -MMD -MP -pthread -D_GNU_SOURCE
# general defines
DEFINES  += -DCPU=$(profile_cpu) -DGPU=$(profile_gpu)
DEFINES  += -DNUM_RUN=$(num_run) -DSAMPLE_PERIOD_US=$(sample_period_us)

# platform-specific
ifeq ($(platform),jetson_agx_xavier)
INC_DIRS += $(CUDA_PATH)/../include $(CUDA_PATH)/include
LIB_DIRS += $(CUDA_PATH)/../lib64 $(CUDA_PATH)/lib64
LIBS     += -lcuda -lcudart -lcupti
FLAGS    +=
DEFINES  += -D__JETSON_AGX_XAVIER
endif

INCLUDES += $(addprefix -I,$(INC_DIRS))
LIBS     += $(addprefix -L,$(LIB_DIRS))
# combine all flags
CFLAGS   ?= $(LIBS) $(INCLUDES) $(FLAGS) $(DEFINES)

# targets
all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(CFLAGS)

# C source
$(BUILD_DIR)/%.o: %.c $(CONFIG_DIR)/config.mk $(VOLTMETER_YML) $(VOLTMETER_MK)
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

# config
$(VOLTMETER_MK):
	$(MAKE) -C $(ROOT_DIR) $(VOLTMETER_MK)

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) $(TARGET)

-include $(DEPS)
