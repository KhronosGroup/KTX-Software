#!/bin/bash

# exit if any command fails
set -e

# Explicitely take newer CMake installed from apt.kitware.com
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


# Emscripten/WebAssembly


# Temporary solution: Update CMake. Can be dropped once the Ubuntu version in the docker
# container is updated.
echo "Update CMake for Web"
docker exec -it emscripten sh -c "apt-get update"
docker exec -it emscripten sh -c "apt-get -qq install -y --no-install-recommends apt-transport-https ca-certificates gnupg software-properties-common wget"
docker exec -it emscripten sh -c "wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null"
docker exec -it emscripten sh -c "apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'"
docker exec -it emscripten sh -c "apt-get update"
docker exec -it emscripten sh -c "apt-get -qq install -y --no-install-recommends cmake"


echo "Configure/Build KTX-Software (Web Debug)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-debug . && cmake --build build-web-debug --config Debug"
echo "Configure/Build KTX-Software (Web Release)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-release . && cmake --build build-web-release --config Release"

echo "Pack KTX-Software (Web Release)"
docker exec -it -w build-web-release emscripten sh -c "cpack -G ZIP"
