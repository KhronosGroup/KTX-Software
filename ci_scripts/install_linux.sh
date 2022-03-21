#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Install software in CI environment necessary to build on Linux.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-ON}

sudo apt-get -qq update
sudo apt-get -qq install ninja-build
sudo apt-get -qq install doxygen
sudo apt-get -qq install rpm
sudo apt-get -qq install opencl-c-headers
sudo apt-get -qq install mesa-opencl-icd

if [ "$FEATURE_LOADTESTS" = "ON" ]; then
  sudo apt-get -qq install libsdl2-dev
  sudo apt-get -qq install libgl1-mesa-glx libgl1-mesa-dev
  sudo apt-get -qq install libvulkan1 libvulkan-dev
  sudo apt-get -qq install libassimp5 libassimp-dev

  echo "Download Vulkan SDK"
  wget -O - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
  sudo wget -O /etc/apt/sources.list.d/lunarg-vulkan-$VULKAN_SDK_VER-focal.list https://packages.lunarg.com/vulkan/$VULKAN_SDK_VER/lunarg-vulkan-$VULKAN_SDK_VER-focal.list
  echo "Install Vulkan SDK"
  sudo apt update
  sudo apt install vulkan-sdk
fi

git lfs pull --include=tests/srcimages,tests/testimages
