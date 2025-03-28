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

if [[ (-n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF") || ("$FEATURE_TESTS" = "ON") ]]; then
  git lfs pull --include=tests/srcimages,tests/testimages
fi

if [[ -n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF" ]]; then
  if [ "$PLATFORM" = "iOS" ]; then
    IOS_COMPONENT=com.lunarg.vulkan.ios
  fi

  if [[ "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
    # Current dir. is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
    pushd ../..
    VULKAN_SDK_NAME=vulkansdk-macos-$VULKAN_SDK_VER
    curl -s -S -o $VULKAN_SDK_NAME.dmg https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/$VULKAN_SDK_NAME.dmg?Human=true
    hdiutil attach $VULKAN_SDK_NAME.dmg
    sudo /Volumes/$VULKAN_SDK_NAME/InstallVulkan.app/Contents/MacOS/InstallVulkan --root "$VULKAN_INSTALL_DIR" --accept-licenses --default-answer --confirm-command install $IOS_COMPONENT
    #hdiutil detach /Volumes/VulkanSDK
    set +e
    while hdiutil detach /Volumes/$VULKAN_SDK_NAME; es=$?; [[ $ss -eq 16 ]]; do
        lsof /Volumes/$VULKAN_SDK_NAME
        sleep 10
    done
    rm $VULKAN_SDK_NAME.dmg
    unset VULKAN_SDK_NAME IOS_COMPONENT
    popd
  fi
fi
