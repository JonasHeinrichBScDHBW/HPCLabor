#!/bin/bash

# Python 3 is already installed

# Install dependencies
snap install code --classic &&
    snap install go --classic && \
    sudo apt-get update && sudo apt-get upgrade -y && \
    sudo apt-get install -y build-essential openmpi-bin mpich git linux-tools-`uname -r` cmake

# Build perforator
cd perforator
make build
cd ..

# Build google benchmark
cd google-benchmark
git clone https://github.com/google/googletest.git googletest
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
sudo cmake --build "build" --config Release --target install
cd ..

# Install paraview manually
# https://www.paraview.org/download/
