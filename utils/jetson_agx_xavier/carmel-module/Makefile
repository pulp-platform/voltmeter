# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Thomas Benz, ETH Zurich <tbenz@iis.ee.ethz.ch>
#         Björn Forsberg, ETH Zurich <bjoernf@iis.ee.ethz.ch>
#         Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

-include ../../../../config.mk

KERNEL_SOURCE = /usr/src/linux-headers-4.9.253-tegra-ubuntu18.04_aarch64/kernel-4.9

name = voltmeter
obj-m += $(name).o
kmod = $(name).ko

all: $(kmod)

$(kmod): $(name).c
	make ARCH=arm64 -C $(KERNEL_SOURCE) M=$(PLATFORM_DIR)/carmel-module modules

install: $(kmod)
	sudo insmod $<
	sudo touch install

clean:
	make ARCH=arm64 -C $(KERNEL_SOURCE) M=$(PLATFORM_DIR)/carmel-module clean
	sudo rm -f install
	sudo rmmod $(kmod) || true
