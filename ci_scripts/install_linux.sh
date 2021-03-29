#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# exit if any command fails
set -e

sudo apt-get -qq update
sudo apt-get -qq install libzstd-dev
sudo apt-get -qq install ninja-build
sudo apt-get -qq install doxygen
sudo apt-get -qq install libsdl2-dev
sudo apt-get -qq install libgl1-mesa-glx libgl1-mesa-dev
sudo apt-get -qq install libvulkan1 libvulkan-dev
sudo apt-get -qq install libassimp5 libassimp-dev
sudo apt-get -qq install rpm

wget -O - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -O /etc/apt/sources.list.d/lunarg-vulkan-$VULKAN_SDK_VER-focal.list https://packages.lunarg.com/vulkan/$VULKAN_SDK_VER/lunarg-vulkan-$VULKAN_SDK_VER-focal.list
echo "Install Vulkan SDK"
sudo apt update
sudo apt install vulkan-sdk

pip3 install reuse
