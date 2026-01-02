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
VULKAN_SDK_VERSION=${VULKAN_SDK_VERSION:-1.4.313.1}
VULKAN_INSTALL_DIR=${VULKAN_INSTALL_DIR:-~/VulkanSDK}

git lfs install
git lfs version
gem install xcpretty

if [[ (-n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF") || ("$FEATURE_TESTS" = "ON") ]]; then
  git lfs pull --include=tests/srcimages,tests/testimages
fi

if [[ -n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF" ]]; then
  if [ "$PLATFORM" = "iOS" ]; then
    ios_component=com.lunarg.vulkan.ios
  fi

  if [[ "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
    pushd ~/Downloads
    vulkan_sdk_dl_name=vulkan_sdk.zip # Name to download
    vulkan_sdk_name=vulkansdk-macos-$VULKAN_SDK_VERSION  # Name after unzip.
    echo curl -s -S -O https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/mac/$vulkan_sdk_dl_name?Human=true
    curl -s -S -O https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/mac/$vulkan_sdk_dl_name?Human=true
    unzip  $vulkan_sdk_dl_name
    open -W $vulkan_sdk_name.app --args --root "$VULKAN_INSTALL_DIR" --accept-licenses --default-answer --confirm-command install $ios_component
    rm $vulkan_sdk_dl_name
    rm -rf $vulkan_sdk_name.app
    unset vulkan_sdk_dw_name vulkan_sdk_name ios_component
    popd
  fi
fi
