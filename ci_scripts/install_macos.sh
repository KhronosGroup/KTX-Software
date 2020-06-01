#!/bin/sh

# exit if any command fails
set -e

brew update > /dev/null
brew install git-lfs
git lfs install
git lfs version
brew install doxygen
brew install sdl2
brew link sdl2

# Current directory is .../build/{KhronosGroup,msc-}/KTX-Software. cd to 'build'.
pushd ../..
wget -O vulkansdk-macos-$VULKAN_SDK_VER.tar.gz https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.tar.gz?Human=true
tar -xzf vulkansdk-macos-$VULKAN_SDK_VER.tar.gz
wget -O Packages.dmg http://s.sudre.free.fr/Software/files/Packages.dmg
hdiutil attach Packages.dmg
sudo installer -pkg /Volumes/Packages*/packages/packages.pkg -target /
hdiutil detach /Volumes/Packages*
popd
