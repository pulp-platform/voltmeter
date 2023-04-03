#!/usr/bin/env bash

echo "Setting maximum power configuration..."
sudo nvpmodel -m 0
sudo jetson_clocks --fan
