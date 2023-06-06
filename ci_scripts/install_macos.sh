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

FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL+Vulkan}
PLATFORM=${PLATFORM:-macOS}
VULKAN_SDK_VER=${VULKAN_SDK_VER:-1.3.243.0}

git lfs install
git lfs version
gem install xcpretty

git lfs pull --include=tests/srcimages,tests/testimages

if [[ -n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF" ]]; then
  if [ "$PLATFORM" = "macOS" ]; then
    git lfs pull --include=other_lib/mac
  else
    git lfs pull --include=other_lib/ios
  fi

  if [[ "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
    # Current dir. is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
    pushd ../..
    curl -s -S -o vulkansdk-macos-$VULKAN_SDK_VER.dmg https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.dmg?Human=true
    hdiutil attach vulkansdk-macos-$VULKAN_SDK_VER.dmg
    sudo /Volumes/VulkanSDK/InstallVulkan.app/Contents/MacOS/InstallVulkan --root "$VULKAN_INSTALL_DIR" --accept-licenses --default-answer --confirm-command install
    #hdiutil detach /Volumes/VulkanSDK
    set +e
    while hdiutil detach /Volumes/VulkanSDK; es=$?; [[ $ss -eq 16 ]]; do
        lsof /Volumes/VulkanSDK
        sleep 10
    done
    rm vulkansdk-macos-$VULKAN_SDK_VER.dmg
    popd
  fi
fi
