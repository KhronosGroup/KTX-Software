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
gem install xcpretty

# Current directory is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
pushd ../..
wget -O vulkansdk-macos-$VULKAN_SDK_VER.dmg https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.dmg?Human=true
hdiutil attach vulkansdk-macos-$VULKAN_SDK_VER.dmg
sudo /Volumes/vulkansdk-macos-$VULKAN_SDK_VER/InstallVulkan.app/Contents/macOS/InstallVulkan --root "$VULKAN_INSTALL_DIR" --accept-licenses --default-answer --confirm-command install
hdiutil detach /Volumes/vulkansdk-macos-$VULKAN_SDK_VER
rm vulkansdk-macos-$VULKAN_SDK_VER.dmg
popd
