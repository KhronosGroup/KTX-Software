#!/bin/bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# exit if any command fails
set -e

# Explicitly take newer CMake installed from apt.kitware.com
CMAKE_EXE=/usr/bin/cmake


# Linux

echo "Configure KTX-Software (Linux Debug)"
${CMAKE_EXE} . -G Ninja -Bbuild-linux-debug -DCMAKE_BUILD_TYPE=Debug -DKTX_FEATURE_LOADTEST_APPS=ON
pushd build-linux-debug
echo "Build KTX-Software (Linux Debug)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Debug)"
ctest # --verbose
popd

echo "Configure KTX-Software (Linux Release)"
${CMAKE_EXE} . -G Ninja -Bbuild-linux-release -DCMAKE_BUILD_TYPE=Release -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
pushd build-linux-release
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
${CMAKE_EXE} . -G Ninja -Bbuild-linux-debug-nosse -DCMAKE_BUILD_TYPE=Debug -DBASISU_SUPPORT_SSE=OFF
pushd build-linux-debug-nosse
echo "Build KTX-Software (Linux Debug without SSE support)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Debug without SSE support)"
ctest # --verbose
popd

echo "Configure KTX-Software (Linux Release without SSE support)"
${CMAKE_EXE} . -G Ninja -Bbuild-linux-release-nosse -DCMAKE_BUILD_TYPE=Release -DBASISU_SUPPORT_SSE=OFF
pushd build-linux-release-nosse
echo "Build KTX-Software (Linux Release without SSE support)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux Release without SSE support)"
ctest # --verbose
popd

# Verify licensing meets REUSE standard.
reuse lint


# Emscripten/WebAssembly

ci_scripts/build_wasm_docker.sh
