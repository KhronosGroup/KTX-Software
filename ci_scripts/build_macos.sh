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

#
# macOS
#

echo "Configure KTX-Software (macOS)"
if [ -n "$MACOS_CERTIFICATES_P12" ]; then
  cmake -GXcode -Bbuild-macos \
  -DKTX_FEATURE_DOC=ON \
  -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}" \
  -DXCODE_CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY}" \
  -DXCODE_DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM}" \
  -DPRODUCTBUILD_IDENTITY_NAME="${PKG_SIGN_IDENTITY}"
else # No secure variables means a PR or fork build.
  cmake -GXcode -Bbuild-macos \
  -DKTX_FEATURE_DOC=ON \
  -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}"
fi

echo "Configure KTX-Software (macOS) without SSE support"
cmake -GXcode -Bbuild-macos-nosse -DBASISU_SUPPORT_SSE=OFF

pushd build-macos

# Build and test Debug
echo "Build KTX-Software (macOS Debug)"
cmake --build . --config Debug -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
echo "Test KTX-Software (macOS Debug)"
ctest -C Debug # --verbose

# Build and test Release
echo "Build KTX-Software (macOS Release)"
if [ -n "$MACOS_CERTIFICATES_P12" ]; then
  cmake --build . --config Release
else
  cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
fi
echo "Test KTX-Software (macOS Release)"
ctest -C Release # --verbose
echo "Install KTX-Software (macOS Release)"
cmake --install . --config Release --prefix ../install-macos-release
echo "Pack KTX-Software (macOS Release)"
if ! cpack -G productbuild; then
  cat _CPack_Packages/Darwin/productbuild/ProductBuildOutput.log
  exit 1
fi

popd

pushd build-macos-nosse

echo "Build KTX-Software (macOS without SSE support Debug)"
cmake --build . --config Debug -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
echo "Test KTX-Software (macOS without SSE support Debug)"
ctest -C Debug # --verbose
echo "Build KTX-Software (macOS without SSE support Release)"
cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
echo "Test KTX-Software (macOS without SSE support Release)"
ctest -C Release # --verbose

popd

#
# iOS
#

echo "Configure KTX-Software (iOS)"
cmake -GXcode -Bbuild-ios -DCMAKE_SYSTEM_NAME=iOS -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}"
pushd build-ios
echo "Build KTX-Software (iOS Debug)"
cmake --build . --config Debug  -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
# echo "Build KTX-Software (iOS Simulator Debug)"
# cmake --build . --config Debug -- -sdk iphonesimulator
echo "Build KTX-Software (iOS Release)"
cmake --build . --config Release -- -sdk iphoneos CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
# echo "Build KTX-Software (iOS Simulator Release)"
# cmake --build . --config Release -- -sdk iphonesimulator
popd
