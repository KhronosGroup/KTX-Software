#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# exit if any command fails
set -e

brew update > /dev/null
brew install git-lfs
git lfs install
git lfs version
brew install doxygen
brew install sdl2
brew link sdl2

# Current directory is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
pushd ../..
wget -O vulkansdk-macos-$VULKAN_SDK_VER.dmg https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.dmg?Human=true
hdiutil attach vulkansdk-macos-$VULKAN_SDK_VER.dmg
popd
