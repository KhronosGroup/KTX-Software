#! /usr/bin/env bash
# Copyright 2023 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Smudge all files with proper $Date$ values.

# Exit if any command fails.
set -e

# Make sure we're in the repo root
cd $(dirname $0)/..


target_files=(pkgdoc/pages.md
    lib/libktx_mainpage.md
    tools/ktxtools_mainpage.md
    interface/js_binding/ktx_wrapper.cpp
)

#for file in "${target_files[@]}"; do
#  rm $file
#  git checkout $file
#done
rm "${target_files[@]}"
git checkout  "${target_files[@]}"

