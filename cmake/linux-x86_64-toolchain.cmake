# Copyright 2023 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# CMake Toolchain file for cross-compiling to x86_64 (amd64) on
# a non-x86_64 device running Linux (Ubuntu).
#
# The following packages must be installed for compiling to x86_64.
#   sudo apt-get -qq install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu binutils-x86-64-linux-gnu

######################################################################
# Nota Bene
#
# Untested. Was under development when Travis-CI made arm64 Ubuntu
# runners available rendering it unneeded. Kept here to preserve the
# learning and in case it becomes useful.
######################################################################

# A note about the platform/architecture naming.
#
# The Ubuntu/Deb packages with the compiler and tools use "x64-64" as
# you can see above. The compiler and tools are named with "x86_64"
# as you will see in the macro settings below. Probably this is done
# to maintain consistency with existing naming conventions for
# packages (no underscores) and tools (use the offical architecture
# name).

# Target operating system name.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Name of C compiler.
set(CMAKE_C_COMPILER "/usr/bin/x86_64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/x86_64-linux-gnu-g++")

# Where to look for the target environment. (More paths can be added here)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-linux-gnu)
#set(CMAKE_SYSROOT /usr/x86_64-linux-gnu)

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
