#!/bin/sh

# exit if any command fails
set -e

echo "Configure KTX-Software (Linux Debug)"
cmake . -G Ninja -Bbuild-linux-debug -DCMAKE_BUILD_TYPE=Debug
echo "Build KTX-Software (Linux Debug)"
cmake --build build-linux-debug
echo "Configure KTX-Software (Linux Release)"
cmake . -G Ninja -Bbuild-linux-release -DCMAKE_BUILD_TYPE=Release -DKTX_FEATURE_DOC=ON
echo "Build KTX-Software (Linux Debug)"
cmake --build build-linux-release
echo "Test KTX-Software (Linux Debug)"
cmake --build build-linux-debug --target test
echo "Test KTX-Software (Linux Release)"
cmake --build build-linux-release --target test
echo "Configure/Build KTX-Software (Web Debug)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-debug . && cmake --build build-web-debug --config Debug"
echo "Configure/Build KTX-Software (Web Release)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-release . && cmake --build build-web-release --config Release"