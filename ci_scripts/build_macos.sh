#!/bin/sh

# exit if any command fails
set -e
set -x

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
if [ -z "$TRAVIS_PULL_REQUEST" -o "$TRAVIS_PULL_REQUEST" = "false" ]; then
  cmake -GXcode -Bbuild-macos \
  -DKTX_FEATURE_DOC=ON \
  -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}" \
  -DXCODE_CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY}" \
  -DXCODE_DEVELOPMENT_TEAM="${DEVELOPMENT_TEAM}" \
  -DPRODUCTBUILD_IDENTITY_NAME="${PKG_SIGN_IDENTITY}"
else # Generally no secure variables in a PR build.
  cmake -GXcode -Bbuild-macos \
  -DKTX_FEATURE_DOC=ON \
  -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}"
fi

pushd build-macos

# Build and test Debug
echo "Build KTX-Software (macOS Debug)"
cmake --build . --config Debug -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
echo "Test KTX-Software (macOS Debug)"
ctest -C Debug # --verbose

# Build and test Release
echo "Build KTX-Software (macOS Release)"
if [ -z "$TRAVIS_PULL_REQUEST" -o "$TRAVIS_PULL_REQUEST" = "false" ]; then
  cmake --build . --config Release
else
  cmake --build . --config Release -- CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO
fi
echo "Test KTX-Software (macOS Release)"
ctest -C Release # --verbose
echo "Install KTX-Software (macOS Release)"
cmake --install . --config Release --prefix ../install-macos-release
echo "Pack KTX-Software (macOS Release)"
cpack -G productbuild

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
