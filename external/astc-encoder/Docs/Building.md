# Building ASTC Encoder

This page provides instructions for building `astcenc` from the sources in
this repository.

Builds must use CMake 3.15 or higher as the build system generator. The
examples on this page show how to use it to generate build systems for NMake
(Windows) and Make (Linux and macOS), but CMake supports other build system
backends.

## Windows

Builds for Windows are tested with CMake 3.17, and Visual Studio 2019 or newer.

### Configuring the build

To use CMake you must first configure the build. Create a build directory in
the root of the `astcenc` checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Create a build directory
mkdir build
cd build

# Configure your build of choice, for example:

# x86-64 using a Visual Studio solution
cmake -G "Visual Studio 16 2019" -T ClangCL -DCMAKE_INSTALL_PREFIX=..\ ^
    -DASTCENC_ISA_AVX2=ON -DASTCENC_ISA_SSE41=ON -DASTCENC_ISA_SSE2=ON ..

# x86-64 using NMake
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=..\ ^
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

## macOS and Linux using Make

Builds for macOS and Linux are tested with CMake 3.17, and clang++ 9.0 or
newer.

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
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../ ..
```

A single CMake configure can build multiple binaries for a single target CPU
architecture, for example building x64 for both SSE2 and AVX2. Each binary name
will include the build variant as a postfix. It is possible to build any set of
the supported SIMD variants by enabling only the ones you require.

For macOS, we additionally support the ability to build a universal binary.
This build includes SSE4.1 (`x86_64`), AVX2 (`x86_64h`), and NEON (`arm64`)
build slices in a single output binary. The OS will select the correct variant
to run for the machine being used. This is the default build target for a macOS
build, but single-target binaries can still be built by setting
`-DASTCENC_UNIVERSAL_BINARY=OFF` and then manually selecting the specific ISA
variants that are required.

### Building

Once you have configured the build you can use Make to compile the project from
your build dir, and install to your target install directory.

```shell
# Run a build and install build outputs in `${CMAKE_INSTALL_PREFIX}/bin/`
# for executable binaries and `${CMAKE_INSTALL_PREFIX}/lib/` for libraries
cd build
make install -j16
```

## macOS using Xcode

Builds for macOS and Linux are tested with CMake 3.17, and Xcode 14.0 or
newer.

### Configuring the build

To use CMake you must first configure the build. Create a build directory
in the root of the astcenc checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Create a build directory
mkdir build
cd build

# Configure a universal build
cmake -G Xcode -DCMAKE_INSTALL_PREFIX=../ ..
```

### Building

Once you have configured the build you can use CMake to compile the project
from your build dir, and install to your target install directory.

```shell
cmake --build . --config Release

# Optionally install the binaries to the installation directory
cmake --install . --config Release
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
option `-DASTCENC_SHAREDLIB=ON` at configure time. For macOS build targets the
shared library supports the same universal build configuration as the command
line utility.

Note that the command line tool is always statically linked; the shared objects
are an extra build output that are not currently used by the command line tool.

### Constrained block size builds

All normal builds will support all ASTC block sizes, including the worst case
6x6x6 3D block size (216 texels per block). Compressor memory footprint and
performance can be improved by limiting the block sizes supported in the build
by adding `-DASTCENC_BLOCK_MAX_TEXELS=<texel_count>` to the CMake command line
when configuring. Legal block sizes that are unavailable in a restricted build
will return the error `ASTCENC_ERR_NOT_IMPLEMENTED` during context creation.

If your use case only needs to support the 2D texture format, we recommend
that you use `-DASTCENC_BLOCK_MAX_TEXELS=144` which will support all 2D block
sizes.

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

### No x86 gather instruction builds

On many x86 microarchitectures the native AVX gather instructions are slower
than simply performing manual scalar loads and combining the results. Gathers
are enabled by default, but can be disabled by setting the CMake option
`-DASTCENC_X86_GATHERS=OFF` on the command line when configuring.

Note that we have seen mixed results when compiling the scalar fallback path,
so we would recommend testing which option works best for the compiler and
microarchitecture pairing that you are targeting.

### Test builds

We support building unit tests. These use the `googletest` framework, which is
pulled in through a git submodule. On first use, you must fetch the submodule
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

### Sanitizer builds

We support building with sanitizers on Linux and macOS when using Clang.

To build binaries with ASAN checking enabled add `-DASTCENC_ASAN=ON` to the
CMake command line when configuring.

To build binaries with UBSAN checking enabled add `-DASTCENC_UBSAN=ON` to the
CMake command line when configuring.

### Android builds

Builds of the command line utility for Android are not officially supported,
but can be a useful development build for testing on e.g. different Arm CPU
microarchitectures.

The build script below shows one possible route to building the command line
tool for Android. Once built the application can be pushed to, e.g.,
`/data/local/tmp` and executed from an Android shell terminal over `adb`.

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

### Enabling the VLEN=256 RISC-V Vector backend

The current RISC-V Vector backend only supports VLEN=256 hardware, due to
toolchain limitations that make a vector-length-agnostic port challenging.
Hopefully there will be one in the future.

Since the backend only works when the vector length is exactly 256 bits, and
you can only enable this as a global compiler flag, there is no separate build
target for the backend yet. To enable it, simply build the NONE or NATIVE build
target, with the `-mrvv-vector-bits=zvl` CXXFLAG and a `zvl256b` in the
`-march` ISA configuration string.

You can cross-compile to RISC-V with the backend enabled using the following
commands:

```bash
export CXX=riscv64-linux-gnu-g++
export CXXFLAGS="-march=rv64gcv_zvl256b -mrvv-vector-bits=zvl"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DASTCENC_ISA_NONE=ON ..
cmake --build build -j$(nproc)
```

See also [.github/workflows](../.github/workflows) for how the CI builds and
runs the unit tests.

## Packaging a release bundle

We support building a release bundle of all enabled binary configurations in
the current CMake configuration using the `package` build target

Configure CMake with:

* `-DASTCENC_PACKAGE=<arch>` to set the package architecture/variant name used
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
integrated into other projects. We recommend integrating the project in a way
that lets you control which version of `astcenc` you are using, as this allows
you to manage API version changes.

A basic example of CMake integration and codec API usage can be found in the `./Utils/Example` directory. See the [Example Readme](../Utils/Example/README.md) for more details.

> [!CAUTION]
> This example is designed to always use the latest main branch so that it
> stays in sync with any changes we make to the repository. This is not
> recommended for external production integration because you cannot control
> which API version your application is using.

### Library API stability across versions

Since version 5.4.0, the library API exposed in `astcenc.h` will follow
semantic versioning guidelines and remain backwards compatible within each
major version (`X.*.*`). Any breaking change will result in a major version
increment.

When updating to a newer major version, callers of the API should expect that
they will need to update to synchronize with any changes. When a major API
version makes changes to the API, they will be done in a way that deliberately
causes a compilation failure. This avoids applications hitting failures that
only occur at runtime.

> [!IMPORTANT]
> Prior to version 5.4.0, we did not follow this policy and minor versions
> (`*.X.*`) could introduce API breaks.

When we make changes that modify API entrypoints, we do not usually try to
maintain backwards compatibility by adding parallel entrypoints to support both
the new and original behaviors, and prefer to just increment the major version.

## Configuring a new build machine

Install native packages:

```shell
sudo apt update

# Build tools
sudo apt install clang cmake git

# Test tools
sudo apt install imagemagick python3-pip

# Profile tools
sudo apt install graphviz valgrind
```

Install Python modules from within a virtual environment:

```shell
# Setup virtual environment
python3 -m venv pve
. ./pve/bin/activate

# Test tools
python3 -m pip install numpy pillow pycodestyle pylint

# Profile tools
python3 -m pip install gprof2dot
```

- - -

_Copyright © 2019-2026, Arm Limited and contributors._
