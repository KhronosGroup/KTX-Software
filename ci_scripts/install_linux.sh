#!/bin/sh

# exit if any command fails
set -e

sudo apt-get -qq update
sudo apt-get -qq install libzstd-dev
sudo apt-get -qq install ninja-build
sudo apt-get -qq install doxygen
sudo apt-get -qq install libsdl2-dev
sudo apt-get -qq install libgl1-mesa-glx libgl1-mesa-dev
sudo apt-get -qq install libvulkan1 libvulkan-dev
sudo apt-get -qq install libassimp4 libassimp-dev
wget -qO - http://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$VULKAN_SDK_VER-xenial.list http://packages.lunarg.com/vulkan/$VULKAN_SDK_VER/lunarg-vulkan-$VULKAN_SDK_VER-xenial.list
sudo apt update
sudo apt install lunarg-vulkan-sdk
