#!/bin/sh

# exit if any command fails
set -e

# Due to the spaces in the platform names, must use array variables so
# destination args can be expanded to a single word.
OSX_XCODE_OPTIONS=(-alltargets -destination "platform=OS X,arch=x86_64")
IOS_XCODE_OPTIONS=(-alltargets -destination "generic/platform=iOS" -destination "platform=iOS Simulator,OS=latest")
XCODE_CODESIGN_ENV='CODE_SIGN_IDENTITY= CODE_SIGN_ENTITLEMENTS= CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO'
# The following and more is needed if you want to actually sign the code.
# See http://stackoverflow.com/questions/27671854/travis-ci-fails-to-build-with-a-code-signing-error.
#KEY_CHAIN=xcode-build.keychain
#security create-keychain -p travis $KEY_CHAIN
# Make the keychain the default so identities are found
#security default-keychain -s $KEY_CHAIN
## Unlock the keychain
#security unlock-keychain -p travis $KEY_CHAIN
## Set keychain locking timeout to 3600 seconds
#security set-keychain-settings -t 3600 -u $KEY_CHAIN

# Ensure that Vulkan SDK's glslc is in PATH
export PATH="${VULKAN_SDK}/bin:$PATH"

echo "Configure KTX-Software (macOS)"
cmake -GXcode -Bbuild-macos -DKTX_FEATURE_DOC=ON -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
pushd build-macos
echo "Build KTX-Software (macOS Debug)"
cmake --build . --config Debug
echo "Test KTX-Software (macOS Debug)"
ctest -C Debug # --verbose
echo "Build KTX-Software (macOS Release)"
cmake --build . --config Release
echo "Test KTX-Software (macOS Release)"
ctest -C Release # --verbose
popd

echo "Configure KTX-Software (iOS)"
cmake -GXcode -Bbuild-ios -DCMAKE_SYSTEM_NAME=iOS -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON -DVULKAN_SDK="${VULKAN_INSTALL_DIR}/MoltenVK"
pushd build-ios
echo "Build KTX-Software (iOS Debug)"
cmake --build . --config Debug  -- -sdk iphoneos
# echo "Build KTX-Software (iOS Simulator Debug)"
# cmake --build . --config Debug -- -sdk iphonesimulator
echo "Build KTX-Software (iOS Release)"
cmake --build . --config Release -- -sdk iphoneos
# echo "Build KTX-Software (iOS Simulator Release)"
# cmake --build . --config Release -- -sdk iphonesimulator
popd
