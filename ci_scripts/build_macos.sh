#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build for macOS with Xcode.

# Exit if any command fails.
set -e

# Travis CI doesn't have JAVA_HOME for some reason
if [ -z "$JAVA_HOME" ]; then
  echo Setting JAVA_HOME from /usr/libexec/java_home
  export JAVA_HOME=$(/usr/libexec/java_home)
  echo JAVA_HOME is $JAVA_HOME
fi

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
FEATURE_TOOLS=${FEATURE_TOOLS:-ON}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=${SUPPORT_SSE:-ON}
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}

BUILD_DIR=${BUILD_DIR:-build/macos-$ARCH-$CONFIGURATION}

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

echo "Configure KTX-Software (macOS $ARCH $CONFIGURATION) dir=$BUILD_DIR FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS FEATURE_TOOLS=$FEATURE_TOOLS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
if [ -n "$MACOS_CERTIFICATES_P12" ]; then
  cmake -GXcode -B$BUILD_DIR . \
  -D CMAKE_OSX_ARCHITECTURES="$ARCHS" \
  -D KTX_FEATURE_DOC=$FEATURE_DOC \
  -D KTX_FEATURE_JNI=$FEATURE_JNI \
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS \
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL \
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE \
  $(if [ "$ARCH" = "x86_64" ]; then echo -D ISA_SSE41=ON; fi) \
  -D XCODE_CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY}" \
  -D XCODE_DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM}" \
  -D PRODUCTBUILD_IDENTITY_NAME="${PKG_SIGN_IDENTITY}"
else # No secure variables means a PR or fork build.
  echo "************* No Secure variables. ******************"
  cmake -GXcode -B$BUILD_DIR . \
  -D CMAKE_OSX_ARCHITECTURES="$ARCHS" \
  -D KTX_FEATURE_DOC=$FEATURE_DOC \
  -D KTX_FEATURE_JNI=$FEATURE_JNI \
  -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
  -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS \
  -D BASISU_SUPPORT_OPENCL=$SUPPORT_OPENCL \
  -D BASISU_SUPPORT_SSE=$SUPPORT_SSE \
  $(if [ "$ARCH" = "x86_64" ]; then echo -D ISA_SSE41=ON; fi)
fi

# Cause the build pipes below to set the exit to the exit code of the
# last program to exit non-zero.
set -o pipefail

pushd $BUILD_DIR

# Build and test Release
echo "Build KTX-Software (macOS $ARCH $CONFIGURATION) FEATURE_DOC=$FEATURE_DOC FEATURE_JNI=$FEATURE_JNI FEATURE_LOADTESTS=$FEATURE_LOADTESTS FEATURE_TOOLS=$FEATURE_TOOLS SUPPORT_SSE=$SUPPORT_SSE SUPPORT_OPENCL=$SUPPORT_OPENCL"
if [ -n "$MACOS_CERTIFICATES_P12" -a "$CONFIGURATION" = "Release" ]; then
  cmake --build . --config Release | handle_compiler_output
else
  cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO | handle_compiler_output
fi

# Rosetta 2 should let x86_64 tests run on an Apple Silicon Mac hence the -o.
if [ "$ARCH" = "$(uname -m)" -o "$ARCH" = "x64_64" ]; then
  echo "Test KTX-Software (macOS $ARCH $CONFIGURATION)"
  ctest -C Release # --verbose
fi

echo "Install KTX-Software (macOS $ARCH $CONFIGURATION)"
if [ "$PACKAGE" = "YES" ]; then
  cmake --install . --config Release --prefix ../install-macos-release
  echo "Pack KTX-Software (macOS $CONFIGURATION)"
  if ! cpack -G productbuild; then
    cat _CPack_Packages/Darwin/productbuild/ProductBuildOutput.log
    exit 1
  fi
fi

#echo "***** toktx version.h *****"
#pwd
#cat ../tools/toktx/version.h
#echo "****** toktx version ******"
#Release/toktx --version
#echo "***************************"

popd

if [ "$FEATURE_JNI" = "YES" ]; then
  LIBKTX_BINARY_DIR=$(pwd)/$DEPLOY_BUILD_DIR/Release ci_scripts/build_java.sh
fi

