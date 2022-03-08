#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Install software in CI environment necessary to build on macOS.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-ON}

git lfs install
git lfs version
gem install xcpretty

if [ "$FEATURE_LOADTESTS" = "ON" ]; then
  # Current dir. is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
  pushd ../..
  wget -O vulkansdk-macos-$VULKAN_SDK_VER.dmg https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.dmg?Human=true
  hdiutil attach vulkansdk-macos-$VULKAN_SDK_VER.dmg
  sudo /Volumes/vulkansdk-macos-$VULKAN_SDK_VER/InstallVulkan.app/Contents/macOS/InstallVulkan --root "$VULKAN_INSTALL_DIR" --accept-licenses --default-answer --confirm-command install
  hdiutil detach /Volumes/vulkansdk-macos-$VULKAN_SDK_VER
  rm vulkansdk-macos-$VULKAN_SDK_VER.dmg
  popd
fi
