#!/usr/bin/env bash

# Copyright 2023 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sergio Mazzola, ETH Zurich <smazzola@iis.ee.ethz.ch>

echo "Setting maximum power configuration..."
sudo nvpmodel -m 0
sudo jetson_clocks --fan
