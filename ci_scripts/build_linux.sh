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

echo "Configure/Build KTX-Software (Web Debug)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-debug . && cmake --build build-web-debug --config Debug"
echo "Configure/Build KTX-Software (Web Release)"
docker exec -it emscripten sh -c "emcmake cmake -Bbuild-web-release . && cmake --build build-web-release --config Release"

docker exec -it emscripten sh -c "which cmake"
docker exec -it emscripten sh -c "ls -l /usr/local/bin"
docker exec -it emscripten sh -c "if [ -d /opt/cmake/bin ]; then ls -R /opt/cmake/bin; fi"

pushd build-web-release
echo "Pack KTX-Software (Web Release)"
# cpack is not in the Emscripter docker config.
#docker exec -it -w $(pwd)/build-web-release emscripten sh -c "cpack --verbose -G ZIP"
ls -R /home/travis/build
# cpack --verbose -G ZIP
docker exec -it emscripten sh -c "emcmake cmake --build build-web-release --config Release --target package"
popd
