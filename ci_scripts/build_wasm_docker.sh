#!/bin/bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Emscripten/WebAssembly

# Check if emscripten container is already running as CI will already have started it.
if [ "$(docker container inspect -f '{{.State.Status}}' emscripten 2> /dev/null)" != "running" ]
then
  # Create docker container.
  docker run -dit --name emscripten -v $(pwd):/src emscripten/emsdk bash
fi

echo "Emscripten version"
docker exec -it emscripten sh -c "emcc --version"

build_parent_dir=build
web_build_base=$build_parent_dir/web
debug_build_dir=${web_build_base}-debug
release_build_dir=${web_build_base}-release

mkdir -p $build_parent_dir

echo "Install Emscripten 2.0.15 to avoid emscripten issue 13926."
docker exec -t emscripten sh -c "emsdk install 2.0.15 && emsdk activate 2.0.15"
echo "Configure/Build KTX-Software (Web Debug)"
docker exec -it emscripten sh -c "emcmake cmake -B${web_build_base}-debug -DKTX_FEATURE_LOADTEST_APPS=ON . && cmake --build ${web_build_base}-debug --config Debug"
echo "Configure/Build KTX-Software (Web Release)"
docker exec -it emscripten sh -c "emcmake cmake -B${web_build_base}-release -DKTX_FEATURE_LOADTEST_APPS=ON . && cmake --build ${web_build_base}-release --config Release"

echo "Pack KTX-Software (Web Release)"
# Call cmake rather than cpack so we don't need knowledge of the working directory
# inside docker.
docker exec -it emscripten sh -c "cmake --build ${web_build_base}-release --config Release --target package"

docker stop emscripten
docker rm emscripten
