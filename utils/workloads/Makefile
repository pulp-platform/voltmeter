# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

include config/config.mk

RODINIA_TAR_URL ?= https://www.cs.virginia.edu/~skadron/lava/Rodinia/Packages/rodinia_3.1.tar.bz2
RODINIA_TAR     := $(notdir $(RODINIA_TAR_URL))

RODINIA_PATCH := $(PATCH_DIR)/rodinia_3_1_jetson_sharedlibs.patch

all: rodinia

rodinia: rodinia_download rodinia_patch rodinia_omp rodinia_cuda rodinia_data_gen

rodinia_download: Makefile $(RODINIA_PATCH)
	curl -O $(RODINIA_TAR_URL)
	tar -xvf $(RODINIA_TAR)
	rm $(RODINIA_TAR)
	mv $(RODINIA_TAR:%.tar.bz2=%) $(RODINIA_DIR)
	cd $(RODINIA_DIR); \
	make clean

rodinia_patch: rodinia_download
	cd $(RODINIA_DIR); \
	git init; \
	git add *; git commit -m "Init"; \
	git apply --whitespace=fix $(RODINIA_PATCH); \
	git add *; git commit -m "Patch"

rodinia_omp:
	cd $(RODINIA_DIR); make OMP_clean; make OMP

rodinia_cuda:
	cd $(RODINIA_DIR); make CUDA_clean; make CUDA

rodinia_data_gen:
	cd $(RODINIA_DIR)/data/bfs/inputGen; make; ./graphgen 8388608 8M; mv graph8M.txt ../
	cd $(RODINIA_DIR)/data/hotspot/inputGen; make; ./hotspotex; mv *_4096 ../
	cd $(RODINIA_DIR)/data/kmeans/inpuGen; make; ./datagen 1000000; mv 1000000_34.txt ../
	cd $(RODINIA_DIR)/data/hybridsort; for i in {1..200..1}; do cat 500000.txt >> 100000000.txt; done

.PHONY: clean

clean:
	rm -rf $(RODINIA_DIR) $(RODINIA_TAR)
