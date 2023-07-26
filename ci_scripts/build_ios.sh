#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build for iOS with Xcode.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any. This is designed
# to handle args of the form PARAM=value or PARAM="value1 value2 ...".
# Any other form of CL args must be handled first.
for i in "$@"; do
  eval $i
done

# Set defaults
ARCH=${ARCH:-$(uname -m)}
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL+Vulkan}
FEATURE_TESTS=${FEATURE_TESTS:-OFF}
FEATURE_TOOLS=${FEATURE_TOOLS:-OFF}
PACKAGE=${PACKAGE:-NO}
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}
SUPPORT_SSE=OFF
WERROR=${WERROR:-OFF}

BUILD_DIR=${BUILD_DIR:-build/ios}

export VULKAN_SDK=${VULKAN_SDK:-~/VulkanSDK/1.2.198.1/macOS}

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

cmake_args=("-G" "Xcode" \
  "-B" $BUILD_DIR \
  "-D" "CMAKE_SYSTEM_NAME=iOS" \
  "-D" "ASTCENC_ISA_NEON=ON" \
  "-D" "KTX_FEATURE_DOC=$FEATURE_DOC" \
  "-D" "KTX_FEATURE_JNI=$FEATURE_JNI" \
  "-D" "KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS" \
  "-D" "KTX_FEATURE_TESTS=$FEATURE_TESTS" \
  "-D" "KTX_FEATURE_TOOLS=$FEATURE_TOOLS" \
  "-D" "BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL" \
  "-D" "BASISU_SUPPORT_SSE=$SUPPORT_SSE" \
  "-D" "KTX_WERROR=$WERROR"
)
config_display="Configure KTX-Software (iOS): "
for arg in "${cmake_args[@]}"; do
  case $arg in
    "-G") config_display+="Generator=" ;;
    "-B") config_display+="Build Dir=" ;;
    "-D") ;;
    *) config_display+="$arg, " ;;
  esac
done

echo ${config_display%??}
cmake . "${cmake_args[@]}"

pushd $BUILD_DIR

oldifs=$IFS
#; is necessary because `for` is a Special Builtin.
IFS=, ; for config in $CONFIGURATION
do
  IFS=$oldifs # Because of ; IFS set above will still be present.
  echo "Build KTX-Software (iOS $config)"
  cmake --build . --config $config -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO | handle_compiler_output

  #echo "Build KTX-Software (iOS Simulator $config)"
  #cmake --build . --config $config -- -sdk iphonesimulator

  if [ "$config" = "Release" -a "$PACKAGE" = "YES" ]; then
    echo "Pack KTX-Software (iOS $config)"
    if ! cpack -C $config; then
      cat _CPack_Packages/iOS/ZIP/ZipBuildOutput.log
      exit 1
    fi
  fi
done

popd

# vim:ai:ts=4:sts=2:sw=2:expandtab

