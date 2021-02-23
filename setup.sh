#!/bin/bash

# Python 3 is already installed

# Install dependencies
snap install code go --classic && \
    apt-get update && apt-get upgrade -y && \
    apt-get install -y build-essential openmpi-bin mpich git linux-tools-`uname -r`

# Build perforator
cd perforator
make build
cd ..

# Install paraview manually
# https://www.paraview.org/download/
