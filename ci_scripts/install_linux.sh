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
VULKAN_SDK_VER=${VULKAN_SDK_VER:-1.3.243}

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

  os_codename=$(grep -E 'VERSION_CODENAME=[a-zA-Z]+$' /etc/os-release)
  os_codename=${os_codename#VERSION_CODENAME=}

  echo "Download Vulkan SDK"
  sudo wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc > /etc/apt/trusted.gpg.d/lunarg.asc
  sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$VULKAN_SDK_VER-$os_codename.list https://packages.lunarg.com/vulkan/$VULKAN_SDK_VER/lunarg-vulkan-$VULKAN_SDK_VER-$os_codename.list

  echo "Install Vulkan SDK"
  sudo apt update
  sudo apt install vulkan-sdk
fi

git lfs pull --include=tests/srcimages,tests/testimages
