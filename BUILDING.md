<!-- Copyright 2013-2020 Mark Callow -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

Building KTX
============

KTX uses the the [CMake](https://cmake.org) build system. Depending on your
platform and how you configure it, it will create project/build files (e.g. an Xcode project, a Visual Studio solution or Make files) that allow you to
build the software and more (See [CMake generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)).

KTX consist of the following parts

- The `libktx` main library
- Command line tools (for Linux / macOS / Windows)
- Load test applications (for [OpenGL<sup>®</sup> 3](https://www.khronos.org/opengl), [OpenGLES<sup>®</sup>](https://www.khronos.org/opengles) or [Vulkan<sup>®</sup>](https://www.khronos.org/vulkan))
- Documentation

Supported platforms (please to through their specific requirements first)

- [GNU/Linux](#gnulinux)
- [iOS and macOS](#ios-and-macos)
- [Web/Emscripten](#webemscripten)
- [Windows 64-bit](#windows-x64)

> **Note:** Android builds will follow

The minimal way to a build is to clone this repository and run the following in a terminal

```bash
# Navigate to the root of your KTX-Software clone (replace with
# your actual path)
cd /path/to/KTX-Software

# This generates build/project files in the sub-folder `build`
cmake . -B build

# Compile the project
cmake --build build
```

This creates the `libktx` library and the command line tools. To create the complete project generate the project like this:

```bash
cmake . -B build -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON
```

If you need the library to be static, enable the `KTX_FEATURE_STATIC_LIBRARY` setting (always enabled on iOS and Emscripten).

Building
--------

### GNU/Linux

You need to install the following

- [CMake](https://cmake.org)
- gcc and g++ from the [GNU Compiler Collection](https://gcc.gnu.org)
- [GNU Make](https://www.gnu.org/software/make) or [Ninja](https://ninja-build.org) (recommended)
- [Doxygen](#doxygen) (only if generating documentation)

Additional requirements for the load tests applications

- SDL2 development library
- assimp development library
- OpenGL development libraries
- Vulkan development libraries
- [Vulkan SDK](#vulkan-sdk)
- zlib development library

On Ubuntu and Debian these can be installed via

```bash
sudo apt install build-essential cmake libzstd-dev ninja-build doxygen libsdl2-dev libgl1-mesa-glx libgl1-mesa-dev libvulkan1 libvulkan-dev libassimp-dev
```

KTX requires `glslc`, which comes with [Vulkan SDK](#vulkan-sdk) (in sub-
folder `x86_64/bin/glslc`). Make sure the complete path to the tool is in
in your environment's `PATH` variable. If you've followed Vulkan SDK
install instructions for your platform this should already be set up. You
can test it by running

```bash
export PATH=$PATH:/path/to/vulkansdk/x86_64/bin
# Should not fail and output version numbers
glslc --version
```


You should be able then to build like this

```bash
# First either configure a debug build of libktx and the tools
cmake . -G Ninja -B build
# ...or alternatively a release build including all targets
cmake . -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DKTX_FEATURE_LOADTEST_APPS=ON -DKTX_FEATURE_DOC=ON

# Compile the project
cmake --build build
```

### iOS and macOS

You need to install the following

- CMake
- Xcode
- [Doxygen](#doxygen) (only if generating documentation)

For the load tests applications you need to install the [Vulkan SDK](#vulkan-sdk).

Other dependencies (like zstd, SDL2 or the assimp library are included in this repository or come with Xcode).

To build for macOS:

```bash
# This creates an Xcode project at `build/mac/KTX-Software.xcodeproj` containing the libktx and tools targets.
mkdir build
cmake -G Xcode -B build/mac

# If you want to build the load test apps as well, you have to set the `KTX_FEATURE_LOADTEST_APPS` parameter and pass the location where you installed the Vulkan SDK as parameter `VULKAN_INSTALL_DIR`:
cmake -GXcode -Bbuild/mac -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}"

# Compile the project
cmake --build build/mac
```

To sign the applications you need to set the following CMake variables:

| Name | Value |
| :---: | :---: |
| XCODE\_CODE\_SIGN\_IDENTITY | Owner of the _Developer ID Application_ certificate to use for signing |
| XCODE\_DEVELOPMENT\_TEAM | Development team of the certificate owner

To sign the installation package you need to set the following variables:

| Name | Value |
| :---: | :---: |
| PRODUCTBUILD\_IDENTITY\_NAME | Owner of the _Developer ID Installer_ certificate to use for signing |
| PRODUCTBUILD\_KEYCHAIN\_PATH | Path to the keychain file with the certificate. Blank if its in the default keychain.

To build for iOS:

```bash
# This creates an Xcode project at `build/ios/KTX-Software.xcodeproj` containing the libktx targets.
mkdir build # if it does not exist
cmake -G Xcode -B build/ios -DCMAKE_SYSTEM_NAME=iOS

# This creates a project to build the load test apps as well.
cmake -G Xcode -B build/ios -DKTX_FEATURE_LOADTEST_APPS=ON -DVULKAN_INSTALL_DIR="${VULKAN_INSTALL_DIR}"

# Compile the project
cmake --build build -- -sdk iphoneos
```
If using the CMake GUI, when it asks you to specify the generator for the project, you need to check _Specify options for cross-compiling_ and on the next screen make sure _Operating System_ is set to `iOS`.

To sign the applications you need to set the following CMake variables:

| Name | Value |
| :---: | :---: |
| XCODE\_CODE\_SIGN\_IDENTITY | Owner of the _iPhone Developer_ certificate to use for signing |
| XCODE\_DEVELOPMENT\_TEAM | Development team of the certificate owner
| XCODE\_PROVISIONING\_PROFILE | Name of the profile to use.

**NOTE:** the `iphoneos` SDK version gets hardwired into the generated projects. After installing an Xcode update that has the SDK for a new version of iOS, builds will fail. The only way to remedy this is to delete the build folder and regenerate from scratch.

### Web/Emscripten

There are two ways to build the Web version of the software: using Docker or using your own Emscripten installation.

#### Using Docker

Install [Docker Desktop](https://www.docker.com/products/docker-desktop) which is available for GNU/Linux, macOS and Windows.

In the repo root run

```bash
ci_scripts/build_wasm_docker.sh
```

This will build both Debug and Release configurations and will include the load test application. Builds are done with the official Emscripten Docker image. Output will be written to the folders `build/web-{debug,release}`.

If you are using Windows you will need a Unix-like shell such as the one with _Git for Windows_ or one in Windows Subsystem for Linux (WSL) to run this script.

#### Using Your Own Emscripten Installation

Install [Emscripten](https://emscripten.org) and follow the [install instructions](https://emscripten.org/docs/getting_started/downloads.html) closely. After you've set up your emscripten environment in a terminal, run the following:

**Debug:**

```bash
# Configure
emcmake cmake -B build-web-debug . -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build-web-debug --config Debug
```

**Release:**

```bash
# Configure
emcmake cmake -B build-web .

# Build
cmake --build build-web
```

To include the load test application into the build add `-DKTX_FEATURE_LOADTEST_APPS=ON` to either of the above configuration steps.

Web builds create two additional targets:

- `ktx_js`, (libktx javascript wrapper)
- `msc_basis_transcoder_js` (transcoder wrapper)

> **Note:** The libktx wrapper does not use the transcoder wrapper. It directly uses the underlying c++ transcoder.

### Windows x64

CMake can create solutions for Microsoft Visual Studio (2015/2017/2019 are supported by KTX).

> **Note:** This project builds only for 64-bit Windows.

The CMake generators for Visual Studio 2017 and earlier generate projects whose default platform is Windows-x86. Since that is not supported by KTX-Software, the build will fail. To generate a project for x64 when using these earlier generators you must use CMake's `-A` option.

```bash
# -G shown for completeness. Not needed if you are happy
# with the CMake's default selection.
cmake -G "Visual Studio 15 2017" -B build -A x64 .
```

When using a more recent Visual Studio you simply need

```bash
cmake -B build .
```

The NSIS compiler is needed if you intend to build packages.

CMake can include OpenGL ES versions of the KTX loader tests in the
generated solution. To build and run these you need to install an
OpenGL ES emulator. See [below](#opengl-es-emulator-for-windows).

The KTX loader tests use libSDL 2.0.12+. You do not need SDL if you only wish to build the library or tools.

The KTX vulkan loader tests require a [Vulkan SDK](#vulkan-sdk)
and the Open Asset Import Library [`libassimp`](#libassimp). You must
install the former.

#### OpenGL ES Emulator for Windows

The `es1loadtests` and `es3loadtests` targets on Windows require an
OpenGL ES emulator.
[Imagination Technologies PowerVR](https://community.imgtec.com/developers/powervr/graphics-sdk/).
emulator is recommended. Any of the other major emulators listed below could also be used:

* [Qualcomm Adreno](https://developer.qualcomm.com/software/adreno-gpu-sdk/tools)
* [Google ANGLE](https://chromium.googlesource.com/angle/angle/)<sup>*</sup>
* [ARM Mali](http://malideveloper.arm.com/resources/tools/opengl-es-emulator/)

If you want to run the `es1loadtests` you will need to use
Imagination Technologies' PowerVR emulator as that alone supports OpenGL ES
1.1. You must set the CMake configuration variable `OPENGL_ES_EMULATOR` to the directory containing the .lib files of your chosen emulator.

<sup>*</sup>You will need to build ANGLE yourself and copy the libs
and dlls to the appropriate directories under `other_lib/win`. Note
that ANGLE's OpenGL ES 3 support is not yet complete.

Dependencies
------------

### SDL

Builds of SDL are provided in the KTX Git repo. These binaries
were built from a post 2.0.12 changeset given below. However
Standard SDL 2.0.12 works fine everywhere so you can download
binaries from [libsdl.org](https://libsdl.org), if you prefer.

#### macOS Notes

If you wish to use the provided version of SDL in other applications
on your system, you can install the framework. Open a shell and enter
the following command

```bash
cp -R other_lib/mac/<configuration>/SDL2.framework /Library/Frameworks
```

replacing `<configuration>` with your choice of `Debug` or `Release`.

#### Building SDL from source

As noted above, KTX uses a post SDL 2.0.12 changeset, no.
[13845](https://hg.libsdl.org/SDL/rev/210fb62f0c96) in the canonical
Mercurial repo at https://hg.libsdl.org/SDL or the automated GitHub
mirror at https://github.com/spurious/SDL-mirror. Clone the repo,
checkout changeset [13845](https://hg.libsdl.org/SDL/rev/210fb62f0c96)
and follow the SDL build instructions.

Copy the results of your build to the appropriate place under the
`other_lib` directory.

### Vulkan SDK

Download [Vulkan SDK from Lunar G](https://vulkan.lunarg.com/sdk/home).

For Ubuntu (Xenial and Bionic) install packages are available. See [Getting
Started - Ubuntu](https://vulkan.lunarg.com/doc/sdk/1.2.141.2/linux/getting_started_ubuntu.html) for detailed instructions.

For other GNU/Linux distributions a `.tar.gz` file is available. See
[Getting Started - Tarball](https://vulkan.lunarg.com/doc/sdk/1.2.141.2/linux/getting_started.html) for detailed instructions.

For Windows install the Vulkan SDK via the installer.

For iOS and macOS, install the Vulkan SDK by copying the content of the mounted `.dmg` to some location of choice. You need version 1.2.141.2 or later. This SDK contains MoltenVK (Vulkan Portability on Metal) for both iOS and macOS.

### Doxygen

You need this if you want to generate the _libktx_ and _ktxtools_
documentation. You need a minimum of version 1.8.14 to generate
the documentation correctly. You can download binaries and
also find instructions for building it from source at [Doxygen
downloads](http://www.stack.nl/~dimitri/doxygen/download.html). Make
sure the directory containing the `doxygen` executable is in your `$PATH`.

### libassimp

You need to install the Open Asset Import Library [`libassimp`](https://github.com/assimp/assimp) 
on GNU/Linux and macOS if you want to build the KTX vulkan loader tests.
The KTX Git repo has binaries for iOS and Windows. You'll find `libassimp` in the standard package manager on GNU/Linux. On macOS it can be installed via [MacPorts](https://www.macports.org/install.php) or [Brew](https://brew.sh/).

{# vim: set ai ts=4 sts=4 sw=2 expandtab textwidth=75:}
