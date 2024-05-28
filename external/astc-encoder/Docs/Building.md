# Building ASTC Encoder

This page provides instructions for building `astcenc` from the sources in
this repository.

Builds must use CMake 3.15 or higher as the build system generator. The
examples on this page show how to use it to generate build systems for NMake
(Windows) and Make (Linux and macOS), but CMake supports other build system
backends.

## Windows

Builds for Windows are tested with CMake 3.17 and Visual Studio 2019.

### Configuring the build

To use CMake you must first configure the build. Create a build directory in
the root of the `astcenc` checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Create a build directory
mkdir build
cd build

# Configure your build of choice, for example:

# x86-64 using NMake
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=..\ ^
    -DASTCENC_ISA_AVX2=ON -DASTCENC_ISA_SSE41=ON -DASTCENC_ISA_SSE2=ON ..

# x86-64 using Visual Studio solution
cmake -G "Visual Studio 16 2019" -T ClangCL -DCMAKE_INSTALL_PREFIX=..\ ^
    -DASTCENC_ISA_AVX2=ON -DASTCENC_ISA_SSE41=ON -DASTCENC_ISA_SSE2=ON ..
```

A single CMake configure can build multiple binaries for a single target CPU
architecture, for example building x64 for both SSE2 and AVX2. Each binary name
will include the build variant as a postfix. It is possible to build any set of
the supported SIMD variants by enabling only the ones you require.

Using the Visual Studio Clang-CL LLVM toolchain (`-T ClangCL`) is optional but
produces significantly faster binaries than the default toolchain. The C++ LLVM
toolchain component must be installed via the Visual Studio installer.

### Building

Once you have configured the build you can use NMake to compile the project
from your build dir, and install to your target install directory.

```shell
# Run a build and install build outputs in `${CMAKE_INSTALL_PREFIX}/bin/`
cd build
nmake install
```

## macOS and Linux

Builds for macOS and Linux are tested with CMake 3.17 and clang++ 9.0.

> Compiling using g++ is supported, but clang++ builds are faster by ~15%.

### Configuring the build

To use CMake you must first configure the build. Create a build directory
in the root of the astcenc checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Select your compiler (clang++ recommended, but g++ works)
export CXX=clang++

# Create a build directory
mkdir build
cd build

# Configure your build of choice, for example:

# Arm arch64
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ \
    -DASTCENC_ISA_NEON=ON ..

# x86-64
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ \
    -DASTCENC_ISA_AVX2=ON -DASTCENC_ISA_SSE41=ON -DASTCENC_ISA_SSE2=ON ..

# macOS universal binary build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ \
    -DASTCENC_ISA_AVX2=ON -DASTCENC_ISA_NEON=ON ..
```

A single CMake configure can build multiple binaries for a single target CPU
architecture, for example building x64 for both SSE2 and AVX2. Each binary name
will include the build variant as a postfix. It is possible to build any set of
the supported SIMD variants by enabling only the ones you require.

For macOS, we additionally support the ability to build a universal binary,
combining one x86 and one arm64 variant into a single output binary. The OS
will select the correct variant to run for the machine being used to run the
built binary. To build a universal binary select a single x86 variant and a
single arm64 variant, and both will be included in a single output binary. It
is not required, but if `CMAKE_OSX_ARCHITECTURES` is set on the command line
(e.g. by XCode-generated build commands) it will be validated against the other
configuration variant settings.

### Building

Once you have configured the build you can use Make to compile the project from
your build dir, and install to your target install directory.

```shell
# Run a build and install build outputs in `${CMAKE_INSTALL_PREFIX}/bin/`
# for executable binaries and `${CMAKE_INSTALL_PREFIX}/lib/` for libraries
cd build
make install -j16
```

## Advanced build options

For codec developers and power users there are a number of useful features in
the build system.

### Build Types

We support and test the following `CMAKE_BUILD_TYPE` options.

| Value            | Description                                              |
| ---------------- | -------------------------------------------------------- |
| Release          | Optimized release build                                  |
| RelWithDebInfo   | Optimized release build with debug info                  |
| Debug            | Unoptimized debug build with debug info                  |

Note that optimized release builds are compiled with link-time optimization,
which can make profiling more challenging ...

### Shared Libraries

We support building the core library as a shared object by setting the CMake
option `-DASTCENC_SHAREDLIB=ON` at configure time.

Note that the command line tool is always statically linked; the shared objects
are an extra build output that are not currently used by the command line tool.

### Constrained block size builds

All normal builds will support all ASTC block sizes, including the worst case
6x6x6 3D block size (216 texels per block). Compressor memory footprint and
performance can be improved by limiting the block sizes supported in the build
by adding `-DASTCENC_BLOCK_MAX_TEXELS=<texel_count>` to to CMake command line
when configuring. Legal block sizes that are unavailable in a restricted build
will return the error `ASTCENC_ERR_NOT_IMPLEMENTED` during context creation.

### Non-invariant builds

All normal builds are designed to be invariant, so any build from the same git
revision will produce bit-identical results for all compilers and CPU
architectures. To achieve this we sacrifice some performance, so if this is
not required you can specify `-DASTCENC_INVARIANCE=OFF` to enable additional
optimizations. This has most benefit for AVX2 builds where we are able to
enable use of the FMA instruction set extensions.

### No intrinsics builds

All normal builds will use SIMD accelerated code paths using intrinsics, as all
supported target architectures (x86 and arm64) guarantee SIMD availability. For
development purposes it is possible to build an intrinsic-free build which uses
no explicit SIMD acceleration (the compiler may still auto-vectorize).

To enable this binary variant add `-DASTCENC_ISA_NONE=ON` to the CMake command
line when configuring. It is NOT recommended to use this for production; it is
significantly slower than the vectorized SIMD builds.

### Test builds

We support building unit tests. These use the `googletest` framework, which is
pulled in though a git submodule. On first use, you must fetch the submodule
dependency:

```shell
git submodule init
git submodule update
```

To build unit tests add `-DASTCENC_UNITTEST=ON` to the CMake command line when
configuring.

To run unit tests use the CMake `ctest` utility from your build directory after
you have built the tests.

```shell
cd build
ctest --verbose
```

### Address sanitizer builds

We support building with ASAN on Linux and macOS when using a compiler that
supports it. To build binaries with ASAN checking enabled add `-DASTCENC_ASAN=ON`
to the CMake command line when configuring.

### Android builds

Builds of the command line utility for Android are not officially supported, but can be a useful
development build for testing on e.g. different Arm CPU microarchitectures.

The build script below shows one possible route to building the command line tool for Android. Once
built the application can be pushed to e.g. `/data/local/tmp` and executed from an Android shell
terminal over `adb`.

```shell
ANDROID_ABI=arm64-v8a
ANDROID_NDK=/work/tools/android/ndk/22.1.7171670

BUILD_TYPE=RelWithDebInfo

BUILD_DIR=build

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake \
    -DCMAKE_INSTALL_PREFIX=./ \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=${ANDROID_ABI} \
    -DANDROID_ARM_NEON=ON \
    -DANDROID_PLATFORM=android-21 \
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang \
    -DANDROID_TOOLCHAIN=clang \
    -DANDROID_STL=c++_static \
    -DARCH=aarch64 \
    -DASTCENC_ISA_NEON=ON \
    ..

make -j16
```

## Packaging a release bundle

We support building a release bundle of all enabled binary configurations in
the current CMake configuration using the `package` build target

Configure CMake with:

* `-DASTCENC_PACAKGE=<arch>` to set the package architecture/variant name used
to name the package archive (not set by default).

```shell
# Run a build and package build outputs in `./astcenc-<ver>-<os>-<arch>.<fmt>`
cd build
make package -j16
```

Windows packages will use the `.zip` format, other packages will use the
`.tar.gz` format.

## Integrating as a library into another project

The core codec of `astcenc` is built as a library, and so can be easily
integrated into other projects using CMake. An example of the CMake integration
and the codec API usage can be found in the `./Utils/Example` directory in the
repository. See the [Example Readme](../Utils/Example/README.md) for more
details.

- - -

_Copyright © 2019-2023, Arm Limited and contributors. All rights reserved._
