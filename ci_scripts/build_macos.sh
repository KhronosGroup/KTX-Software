#!/bin/sh
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# exit if any command fails
set -e

# Due to the spaces in the platform names, must use array variables so
# destination args can be expanded to a single word.
OSX_XCODE_OPTIONS=(-alltargets -destination "platform=OS X,arch=x86_64")
IOS_XCODE_OPTIONS=(-alltargets -destination "generic/platform=iOS" -destination "platform=iOS Simulator,OS=latest")
XCODE_CODESIGN_ENV='CODE_SIGN_IDENTITY= CODE_SIGN_ENTITLEMENTS= CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO'

# Ensure that Vulkan SDK's glslc is in PATH
export PATH="${VULKAN_SDK}/bin:$PATH"

if [ -z "$DEPLOY_BUILD_DIR" ]; then
  DEPLOY_BUILD_DIR=build-macos-universal
fi

#
# macOS
#

# Since the compiler is called twice (for x86_64 and arm64) with the same set
# of defines and options, we have no choice but to disable SSE. This is done
# by our cpu type detection script (cmake/cputypetest.cmake) which notices the
# multiple architectures and indicates a cpu type that does not support SSE.
echo "Configure KTX-Software (macOS universal binary) without SSE support"
if [ -n "$MACOS_CERTIFICATES_P12" ]; then
  cmake -GXcode -B$DEPLOY_BUILD_DIR \
  -DKTX_FEATURE_DOC=ON \
  -DCMAKE_OSX_ARCHITECTURES="\$(ARCHS_STANDARD)" \
  -DBASISU_SUPPORT_SSE=OFF \
  -DKTX_FEATURE_LOADTEST_APPS=ON \
  -DXCODE_CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY}" \
  -DXCODE_DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM}" \
  -DPRODUCTBUILD_IDENTITY_NAME="${PKG_SIGN_IDENTITY}"
else # No secure variables means a PR or fork build.
  echo "************* No Secure variables. ******************"
  cmake -GXcode -B$DEPLOY_BUILD_DIR \
  -DKTX_FEATURE_DOC=ON \
  -DCMAKE_OSX_ARCHITECTURES="\$(ARCHS_STANDARD)" \
  -DBASISU_SUPPORT_SSE=OFF \
  -DKTX_FEATURE_LOADTEST_APPS=ON
fi

echo "Configure KTX-Software (macOS x86_64) with SSE support"
cmake -GXcode -Bbuild-macos-sse \
  -DCMAKE_OSX_ARCHITECTURES="x86_64" \
  -DBASISU_SUPPORT_SSE=ON -DISA_NONE-ON

# Cause the build pipes below to set the exit to the exit code of the
# last program to exit non-zero.
set -o pipefail

pushd $DEPLOY_BUILD_DIR

# Build and test Debug
echo "Build KTX-Software (macOS universal binary Debug)"
cmake --build . --config Debug -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO | tee fullbuild.log | xcpretty
echo "Test KTX-Software (macOS universal binary Debug)"
ctest -C Debug # --verbose

# Build and test Release
echo "Build KTX-Software (macOS universal binary Release)"
if [ -n "$MACOS_CERTIFICATES_P12" ]; then
  cmake --build . --config Release | tee -a fullbuild.log | xcpretty
else
  cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO | tee -a fullbuild.log | xcpretty
fi
echo "Test KTX-Software (macOS universal binary Release)"
ctest -C Release # --verbose
echo "Install KTX-Software (macOS universal binary Release)"
cmake --install . --config Release --prefix ../install-macos-release
echo "Pack KTX-Software (macOS Release)"
if ! cpack -G productbuild; then
  cat _CPack_Packages/Darwin/productbuild/ProductBuildOutput.log
  exit 1
fi

popd

pushd build-macos-sse

echo "Build KTX-Software (macOS with SSE support Debug)"
cmake --build . --config Debug -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO | tee -a fullbuild.log | xcpretty

echo "Test KTX-Software (macOS with SSE support Debug)"
ctest -C Debug # --verbose
echo "Build KTX-Software (macOS with SSE support Release)"
cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO | tee -a fullbuild.log | xcpretty

echo "Test KTX-Software (macOS with SSE support Release)"
ctest -C Release # --verbose

popd

#
# iOS
#

echo "Configure KTX-Software (iOS)"

cmake -GXcode -Bbuild-ios -DISA_NEON=ON -DCMAKE_SYSTEM_NAME=iOS -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
pushd build-ios
echo "Build KTX-Software (iOS Debug)"
cmake --build . --config Debug  -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO | tee -a fullbuild.log | xcpretty
# echo "Build KTX-Software (iOS Simulator Debug)"
# cmake --build . --config Debug -- -sdk iphonesimulator
echo "Build KTX-Software (iOS Release)"
cmake --build . --config Release -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO | tee -a fullbuild.log | xcpretty
# echo "Build KTX-Software (iOS Simulator Release)"
# cmake --build . --config Release -- -sdk iphonesimulator
popd
