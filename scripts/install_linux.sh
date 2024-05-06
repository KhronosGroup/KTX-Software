#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Install software in CI environment necessary to build on Linux.

# Exit if any command fails.
set -e

# Set parameters from command-line arguments, if any.
for i in $@; do
  eval $i
done

ARCH=${ARCH:-$(uname -m)}  # Architecture to install tools for.
FEATURE_GL_UPLOAD=${FEATURE_GL_UPLOAD:-ON}
FEATURE_VK_UPLOAD=${FEATURE_VK_UPLOAD:-ON}
if [ "$ARCH" = "x86_64" ]; then
  FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL+Vulkan}
else
  # No Vulkan SDK yet for Linux/arm64.
  FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL}
fi
VULKAN_SDK_VER=${VULKAN_SDK_VER:-1.3.243}

sudo apt-get -qq update

# Packages can be specified as 'package:architecture' pretty-much
# anywhere. Use :native to request a package for the build machine.
# See https://wiki.debian.org/Multiarch/HOWTO for information on
# multi-architecture package installs.

# Tools to run on the build host.
# LFS is not preinstalled in the arm64 image.
sudo apt-get -qq install git-lfs:native
sudo apt-get -qq install ninja-build:native
sudo apt-get -qq install doxygen:native
sudo apt-get -qq install rpm:native

if [ "$ARCH" = "$(uname -m)" ]; then
  dpkg_arch=native
  # gcc, g++ and binutils for native builds should already be installed
  # on CI platforms together with cmake.
  # sudo apt-get -qq install gcc g++ binutils make
else
  # Adjust for dpkg/apt architecture naming. How irritating that
  # it differs from what uname -m reports.
  if [ "$ARCH" = "x86_64" ]; then
    dpkg_arch=amd64
    gcc_pkg_arch=x86-64
  elif [ "$ARCH" = "aarch64" ]; then
    dpkg_arch=arm64
    gcc_pkg_arch=$ARCH
  fi
  sudo dpkg --add-architecture $dpkg_arch
  sudo apt-get update
  # Don't think this is right to install cross-compiler. apt reports
  # package not available.
  #sudo apt-get -qq install gcc:$dpkg_arch g++:$dpkg_arch binutils:$dpkg_arch
  # Try this where `arch` is x86-64 or arm64.
  sudo apt-get -qq install gcc-$gcc_pkg_arch-linux-gnu:native g++-$gcc_pkg_arch-linux-gnu:native binutils-$gcc_pkg_arch-linux-gnu:native
fi
sudo apt-get -qq install opencl-c-headers:$dpkg_arch
sudo apt-get -qq install mesa-opencl-icd:$dpkg_arch
if [[ "$FEATURE_GL_UPLOAD" = "ON" || "$FEATURE_LOADTESTS" =~ "OpenGL" ]]; then
  sudo apt-get -qq install libgl1-mesa-glx:$dpkg_arch libgl1-mesa-dev:$dpkg_arch
fi
if [[ "$FEATURE_VK_UPLOAD" = "ON" || "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
  sudo apt-get -qq install libvulkan1 libvulkan-dev:$dpkg_arch
fi
if [[ -n "$FEATURE_LOADTESTS" && "$FEATURE_LOADTESTS" != "OFF" ]]; then
  sudo apt-get -qq install libsdl2-dev:$dpkg_arch
  sudo apt-get -qq install libassimp5 libassimp-dev:$dpkg_arch
fi

if [[ "$FEATURE_LOADTESTS" =~ "Vulkan" ]]; then
  # No Vulkan SDK for Linux/arm64 yet.
  if [[ "$dpkg_arch" = "arm64" ]]; then
    echo "No Vulkan SDK for Linux/arm64 yet. Please set FEATURE_LOADTESTS to OpenGL or OFF."
  else
    os_codename=$(grep -E 'VERSION_CODENAME=[a-zA-Z]+$' /etc/os-release)
    os_codename=${os_codename#VERSION_CODENAME=}

    echo "Download Vulkan SDK"
    # tee is used (and elevated with sudo) so we can write to the destination.
    wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc > /dev/null
    sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$VULKAN_SDK_VER-$os_codename.list https://packages.lunarg.com/vulkan/$VULKAN_SDK_VER/lunarg-vulkan-$VULKAN_SDK_VER-$os_codename.list
    echo "Install Vulkan SDK"
    sudo apt update
    sudo apt install vulkan-sdk
  fi
fi

git lfs pull --include=tests/srcimages,tests/testimages

# vim:ai:ts=4:sts=2:sw=2:expandtab
