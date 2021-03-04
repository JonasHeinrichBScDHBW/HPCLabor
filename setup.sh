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

# Install Intel C/C++ compiler for AVX512
# Commented out: It's uncommonly supported by hardware and large (14GB). Be sure you want this!

# sudo -E apt autoremove intel-hpckit intel-basekit
# cd /tmp
# wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
# sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
# rm GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
# echo "deb https://apt.repos.intel.com/oneapi all main" | sudo tee /etc/apt/sources.list.d/oneAPI.list
# sudo add-apt-repository "deb https://apt.repos.intel.com/oneapi all main"
# sudo apt update
# sudo apt install intel-basekit
