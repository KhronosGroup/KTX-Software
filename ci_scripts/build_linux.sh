#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build on Linux.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

# Set some defaults
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-ON}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=${SUPPORT_SSE:-ON}
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}

BUILD_DIR=${BUILD_DIR:-build/linux-$CONFIGURATION}

mkdir -p $BUILD_DIR

echo "Configure KTX-Software (Linux $CONFIGURATION) dir=$BUILD_DIR FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
cmake . -G Ninja -B$BUILD_DIR \
  -D CMAKE_BUILD_TYPE=$CONFIGURATION \
  -D KTX_FEATURE_DOC=$FEATURE_DOC \
  -D KTX_FEATURE_JNI=$FEATURE_JNI \
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL \
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE
pushd $BUILD_DIR
echo "Build KTX-Software (Linux $CONFIGURATION)"
${CMAKE_EXE} --build .
echo "Test KTX-Software (Linux $CONFIGURATION)"
ctest # --verbose
popd

if [ "$PACKAGE" == "YES" ]; then
  echo "Pack KTX-Software (Linux $CONFIGURATION) FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
  cpack -G DEB
  cpack -G RPM
  cpack -G TBZ2
  popd
fi

#echo "***** toktx version.h *****"
#cat tools/toktx/version.h
#echo "****** toktx version ******"
#build/linux-release/tools/toktx/toktx --version
#echo "***************************"


if [ "$FEATURE_JNI" == "YES" ]; then
  LIBKTX_BINARY_DIR=$(pwd)/$release_build_dir ci_scripts/build_java.sh
fi
