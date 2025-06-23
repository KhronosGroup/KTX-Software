# Building Big Endian ASTC Encoder

We don't officially support the big-endian compressor build, and it is only 
lightly tested, but ensuring that the code is BE-compatible is good practice 
and some of the open source distributions still support BE platforms. 

Even though Arm64 can run in a BE mode, it's now very rare in practice. It's no
longer supported out of the box in the latest Arm upstream compiler releases,
and getting hold of a sysroot is increasingly difficult. To test BE builds, I
therefore cross-compile Linux builds for PPC64 and use `qemu-user` to run
them. This doesn't use a real sysroot, and so everything must be compiled with
`-static` linkage.

## Host software

Install the following host software:

```bash
# Compiler
sudo apt-get install g++-powerpc64-linux-gnu

# Multi-arch libraries
sudo apt-get install g++-multilib-powerpc64-linux-gnu

# QEMU
sudo apt-get install qemu-user-static qemu-user-binfmt binfmt-support
sudo mkdir /etc/qemu-binfmt
sudo ln -s /usr/powerpc64-linux-gnu /etc/qemu-binfmt/ppc64
sudo update-binfmts --import qemu-ppc64
```

## CMake toolchain file

Cross-compiling needs a correctly configured CMake, and the easiest way to
do this consistently is to use a toolchain file. Create a `CMake-BE.toolchain`
file in the root of the project, with the following content:

```
# Operating system
set(CMAKE_SYSTEM_NAME Linux)

# Cross-compilers for C and C++
set(CMAKE_C_COMPILER   powerpc64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER powerpc64-linux-gnu-g++)

# Compiler environment
set(CMAKE_FIND_ROOT_PATH /usr/powerpc64-linux-gnu)

set(CMAKE_C_FLAGS_INIT -static)
set(CMAKE_CXX_FLAGS_INIT -static)
set(CMAKE_EXE_LINKER_FLAGS_INIT -static)
set(CMAKE_SHARED_LINKER_FLAGS_INIT -static)
set(CMAKE_MODULE_LINKER_FLAGS_INIT -static)

# Build options
set(ASTCENC_BIG_ENDIAN ON)

# Never match host tools
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Only match headers and libraries in the compiler environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## Build astcenc

Building uses CMake as normal, with the additional specification of the
toolchain file to configure the build for cross-compilation. We don't have any
SIMD implementations for big-endian architectures so these builds must compile
for the reference C SIMD implementation, `ASTCENC_ISA_NONE`.

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ -DASTCENC_ISA_NONE=ON -DCMAKE_TOOLCHAIN_FILE=../CMake-BE.toolchain ..
```

## Run astcenc

The cross-compiled `astcenc` binary runs as normal, and can access host files,
but must run through QEMU to do the instruction set translation.

If the binfmt setup performed earlier was successful you can just run the
binary as if it were a native binary:

```
./bin/astcenc-none ...
```

... but otherwise you can run it manually using QEMU as a wrapper:

```
qemu-ppc64 ./bin/astcenc-none ...
```

- - -

_Copyright © 2025, Arm Limited and contributors._
