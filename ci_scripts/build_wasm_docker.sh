#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build WebAssembly with Emscripten in Docker.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

# Set defaults
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-ON}
FEATURE_TOOLS=${FEATURE_TOOLS:-OFF}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=OFF
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}

BUILD_DIR=${BUILD_DIR:-build/web-$CONFIGURATION}

# Check if emscripten container is already running as CI will already have started it.
if [ "$(docker container inspect -f '{{.State.Status}}' emscripten 2> /dev/null)" != "running" ]
then
  # Create docker container.
  docker run -dit --name emscripten -v $(pwd):/src emscripten/emsdk bash
else
  echo "Docker running"
  dockerrunning=1
fi

echo "\nEmscripten version"
docker exec -it emscripten sh -c "emcc --version"

mkdir -p $BUILD_DIR

echo "Configure/Build KTX-Software (Web $CONFIGURATION)"
docker exec -it emscripten sh -c "emcmake cmake -B$BUILD_DIR . \
    -D KTX_FEATURE_DOC=OFF \
    -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
  && cmake --build $BUILD_DIR --config $CONFIGURATION"

if [ "$PACKAGE" = "YES" ]; then
  echo "Pack KTX-Software (Web Release)"
  # Call cmake rather than cpack so we don't need knowledge of the working
  # directory inside docker.
  docker exec -it emscripten sh -c "cmake --build $BUILD_DIR --config $CONFIGURATION --target package"
fi

if [ -z $dockerrunning ]; then
  docker stop emscripten > /dev/null
  docker rm emscripten > /dev/null
fi
