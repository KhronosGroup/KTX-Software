#!/bin/bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# exit if any command fails
set -e

# Explicitly take newer CMake installed from apt.kitware.com
CMAKE_EXE=/usr/bin/cmake


# Linux

build_parent_dir=build
linux_build_base=$build_parent_dir/linux
debug_build_dir=${linux_build_base}-debug
release_build_dir=${linux_build_base}-release
nosse_debug_build_dir=${linux_build_base}-nosse-debug
nosse_release_build_dir=${linux_build_base}-nosse-release

mkdir -p $build_parent_dir

echo "Configure KTX-Software (Linux Debug)"
${CMAKE_EXE} . -G Ninja -B$debug_build_dir -DCMAKE_BUILD_TYPE=Debug -DKTX_FEATURE_LOADTEST_APPS=ON
pushd $debug_build_dir
echo "Build KTX-Software (Linux Debug)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Debug)"
ctest # --verbose
popd

echo "Configure KTX-Software (Linux Release)"
${CMAKE_EXE} . -G Ninja -B$release_build_dir -DCMAKE_BUILD_TYPE=Release -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
pushd $release_build_dir
echo "Build KTX-Software (Linux Release)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Release)"
ctest # --verbose
echo "Pack KTX-Software (Linux Release)"
cpack -G DEB
cpack -G RPM
cpack -G TBZ2
popd

echo "Configure KTX-Software (Linux Debug without SSE support)"
${CMAKE_EXE} . -G Ninja -B$nosse_debug_build_dir -DCMAKE_BUILD_TYPE=Debug -DBASISU_SUPPORT_SSE=OFF
pushd $nosse_debug_build_dir
echo "Build KTX-Software (Linux Debug without SSE support)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Debug without SSE support)"
ctest # --verbose
popd

echo "Configure KTX-Software (Linux Release without SSE support)"
${CMAKE_EXE} . -G Ninja -B$nosse_release_build_dir -DCMAKE_BUILD_TYPE=Release -DBASISU_SUPPORT_SSE=OFF
pushd $nosse_release_build_dir
echo "Build KTX-Software (Linux Release without SSE support)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Release without SSE support)"
ctest # --verbose
popd

# Verify licensing meets REUSE standard.
reuse lint


# Emscripten/WebAssembly

ci_scripts/build_wasm_docker.sh
