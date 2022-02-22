# Building ASTC Encoder

This page provides instructions for building `astcenc` from the sources in
this repository.

Builds use CMake 3.15 or higher as the build system generator. The examples on
this page only show how to use it to target NMake (Windows) and Make
(Linux and macOS), but CMake supports other build system backends.

## Windows

Builds for Windows are tested with CMake 3.17 and Visual Studio 2019.

### Configuring the build

To use CMake you must first configure the build. Create a build directory
in the root of the astenc checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Create a build directory
mkdir build
cd build

# Configure your build of choice, for example:

# x86-64 using NMake
cmake -G "NMake Makefiles" -T ClangCL -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX=.\ -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON ..

# x86-64 using Visual Studio solution
cmake -G "Visual Studio 16 2019" -T ClangCL -DCMAKE_INSTALL_PREFIX=.\ ^
    -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON ..
```

This example shows all SIMD variants being enabled. It is possible to build a
subset of the supported variants by enabling only the ones you require. At
least one variant must be enabled.

Using the Visual Studio Clang-cl LLVM toolchain (`-T ClangCL`) is optional but
produces signficantly faster binaries than the default toolchain. The C++ LLVM
toolchain component must be installed via the Visual Studio installer.

### Building

Once you have configured the build you can use NMake to compile the project
from your build dir, and install to your target install directory.

```shell
# Run a build and install build outputs in `${CMAKE_INSTALL_PREFIX}/astcenc/`
cd build
nmake install
```

## macOS and Linux

Builds for macOS and Linux are tested with CMake 3.17 and clang++ 9.0.

> Compiling using g++ is supported, but clang++ builds are faster by ~15%.

### Configuring the build

To use CMake you must first configure the build. Create a build directory
in the root of the astenc checkout, and then run `cmake` inside that directory
to generate the build system.

```shell
# Select your compiler (clang++ recommended, but g++ works)
export CXX=clang++

# Create a build directory
mkdir build
cd build

# Configure your build of choice, for example:

# Arm arch64
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./ \
    -DISA_NEON=ON ..

# x86-64
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./ \
    -DISA_AVX2=ON -DISA_SSE41=ON -DISA_SSE2=ON ..

# macOS universal binary build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./ \
    -DISA_AVX2=ON -DISA_NEON=ON ..
```

This example shows all SIMD variants being enabled. It is possible to build a
subset of the supported variants by enabling only the ones you require.

For all platforms a single CMake configure can build multiple binaries for a
single target CPU architecture, for example building x64 for both SSE2 and
AVX2. The binary name will include the build variant as a postfix.

The macOS platform additionally supports the ability to build a universal
binary, combining one x86 and one arm64 variant into a single output binary.
The OS select the correct variant to run for the machine being used to run the
binary. To build a universal binary select a single x64 variant and a single
arm64 variant, and both will be included in a single output binary. It is not
required, but if `CMAKE_OSX_ARCHITECTURES` is set on the command line (e.g.
by XCode-generated build commands) it will be validated against the other
configuration variant settings.

### Building

Once you have configured the build you can use Make to compile the project from
your build dir, and install to your target install directory.

```shell
# Run a build and install build outputs in `${CMAKE_INSTALL_PREFIX}/astcenc/`
cd build
make install -j16
```

## Advanced build options

For codec developers there are a number of useful features in the build system.

### No intrinsics build

All normal builds will use SIMD accelerated code paths using intrinsics, as all
target architectures (x86-64 and aarch64) guarantee SIMD availability. For
development purposes it is possible to build an intrinsic-free build which uses
no explicit SIMD acceleration (the compiler may still auto-vectorize).

To enable this binary variant add `-DISA_NONE=ON` to the CMake command line
when configuring. It is NOT recommended to use this for production; it is
significantly slower than the vectorized SIMD builds.

### Build Types

We support and test the following `CMAKE_BUILD_TYPE` options.

| Value            | Description                                              |
| ---------------- | -------------------------------------------------------- |
| Release          | Optimized release build                                  |
| RelWithDebInfo   | Optimized release build with debug info                  |
| Debug            | Unoptimized debug build with debug info                  |

Note that optimized release builds are compiled with link-time optimization,
which can make profiling more challenging ...

### Testing

We support building unit tests.

These builds use the `googletest` framework, which is pulled in though a git
submodule. On first use, you must fetch the submodule dependency:

```shell
git submodule init
git submodule update
```

To build unit tests add `-DUNITTEST=ON` to the CMake command line when
configuring.

To run unit tests use the CMake `ctest` utility from your build directory after
you have built the tests.

```shell
cd build
ctest --verbose
```

### Packaging

We support building a release bundle of all enabled binary configurations in
the current CMake configuration using the `package` build target

```shell
# Run a build and package build outputs in `./astcenc-<ver>-<os>-<arch>.<fmt>`
cd build
make package -j16
```

Windows packages will use the `.zip` format, other packages will use the
`.tar.gz` format.

## Integrating as a library into another project

The core codec of astcenc is built as a library, and so can be easily
integrated into other projects using CMake. An example of the CMake integration
and the codec API usage can be found in the `./Utils/Example` directory in the
repository. See the [Example Readme](../Utils/Example/README.md) for more
details.
