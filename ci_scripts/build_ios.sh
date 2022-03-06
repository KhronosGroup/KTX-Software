#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build for iOS with Xcode.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

# Set defaults
ARCH=${ARCH:-$(uname -m)}
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-ON}
FEATURE_TOOLS=${FEATURE_TOOLS:-OFF}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=OFF
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}

BUILD_DIR=${BUILD_DIR:-build/ios-$CONFIGURATION}

export VULKAN_SDK=${VULKAN_SDK:-VULKAN_SDK=~/VulkanSDK/1.2.176.1/macOS}

# Ensure that Vulkan SDK's glslc is in PATH
export PATH="${VULKAN_SDK}/bin:$PATH"

# Due to the spaces in the platform names, must use array variables so
# destination args can be expanded to a single word.
OSX_XCODE_OPTIONS=(-alltargets -destination "platform=OS X,arch=x86_64")
IOS_XCODE_OPTIONS=(-alltargets -destination "generic/platform=iOS" -destination "platform=iOS Simulator,OS=latest")
XCODE_CODESIGN_ENV='CODE_SIGN_IDENTITY= CODE_SIGN_ENTITLEMENTS= CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO'

if which -s xcpretty ; then
  function handle_compiler_output() {
    tee -a fullbuild.log | xcpretty
  }
else
  function handle_compiler_output() {
    cat
  }
fi

# Cause the build pipes below to set the exit to the exit code of the
# last program to exit non-zero.
set -o pipefail

#
# iOS
#

echo "Configure KTX-Software (iOS $CONFIGURATION) dir=$BUILD_DIR FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS FEATURE_TOOLS=$FEATURE_TOOLS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
cmake -GXcode -B$BUILD_DIR \
  -D ISA_NEON=ON \
  -D CMAKE_SYSTEM_NAME=iOS \
  -D KTX_FEATURE_DOC=$FEATURE_DOC \
  -D KTX_FEATURE_JNI=$FEATURE_JNI \
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS \
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL \
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE
pushd $BUILD_DIR

echo "Build KTX-Software (iOS $CONFIGURATION) FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS FEATURE_TOOLS=$FEATURE_TOOLS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
cmake --build . --config $CONFIGURATION -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO | handle_compiler_output

# echo "Build KTX-Software (iOS Simulator)"
#echo "Build KTX-Software (iOS Simulator $CONFIGURATION) FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS SUPPORT_SSE=OFF SUPPORT_OPENCL=$SUPPORT_OPENCL"
# cmake --build . --config $CONFIGURATION -- -sdk iphonesimulator

popd

if [ "$FEATURE_JNI" = "YES" ]; then
  LIBKTX_BINARY_DIR=$(pwd)/$DEPLOY_BUILD_DIR/Release ci_scripts/build_java.sh
fi

