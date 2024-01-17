#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Check generation of VkFormat related files.

BUILD_DIR=${BUILD_DIR:-build/checkmkvk}

cmake . -B $BUILD_DIR -D KTX_FEATURE_TESTS=OFF -D KTX_FEATURE_TOOLS=OFF -D KTX_GENERATE_VK_FILES=ON
# Clean first is to ensure all files are generated so everything is tested.
cmake --build $BUILD_DIR --target mkvk --clean-first
# Verify no files were modified. Exit with 1, if so.
if ! git diff --quiet HEAD; then
    git status
    git diff
    exit 1
fi
