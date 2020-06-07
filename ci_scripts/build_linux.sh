#!/bin/bash

# exit if any command fails
set -e

echo "Configure KTX-Software (Linux Debug)"
cmake . -G Ninja -Bbuild-linux-debug -DCMAKE_BUILD_TYPE=Debug -DKTX_FEATURE_LOADTEST_APPS=ON
pushd build-linux-debug
echo "Build KTX-Software (Linux Debug)"
cmake --build .
echo "Test KTX-Software (Linux Debug)"
ctest # --verbose
popd

echo "Configure KTX-Software (Linux Release)"
cmake . -G Ninja -Bbuild-linux-release -DCMAKE_BUILD_TYPE=Release -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
pushd build-linux-release
echo "Build KTX-Software (Linux Release)"
cmake --build .
echo "Test KTX-Software (Linux Release)"
ctest # --verbose
popd

echo "Configure/Build KTX-Software (Web Debug)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-debug . && cmake --build build-web-debug --config Debug"
echo "Configure/Build KTX-Software (Web Release)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-release . && cmake --build build-web-release --config Release"