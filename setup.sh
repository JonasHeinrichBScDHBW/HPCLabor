#!/bin/bash

# Python 3 is already installed

snap install code --classic && \
    apt-get update && apt-get upgrade -y && \
    apt-get install -y build-essential openmpi-bin mpich git

# Install paraview manually
# https://www.paraview.org/download/
