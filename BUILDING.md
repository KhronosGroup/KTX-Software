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
- [Apple macOS/iOS](#apple-macosios)
- [Web/Emscripten](#webemscripten)
- [Windows](#windows)
- [Android](#android)

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
cmake . -B build -D KTX_FEATURE_LOADTEST_APPS=ON -D KTX_FEATURE_DOC=ON
```

If you need the library to be static, add `-D KTX_FEATURE_STATIC_LIBRARY=ON` to the CMake configure command (always enabled on iOS and Emscripten).

> **Note:**
>
> When linking to the static library, make sure to
> define `KHRONOS_STATIC` before including KTX header files.
> This is especially important on Windows.

If you want the Basis Universal encoders in `libktx` to use OpenCL
add `-D BASISU_SUPPORT_OPENCL=ON` to the CMake configure command.

> **Note:**
> 
>  There is very little advantage to using OpenCL in the context
>  of `libktx`. It is disabled in the default build configuration.


Building
--------

### GNU/Linux

You need to install the following

- [CMake](https://cmake.org)
- gcc and g++ from the [GNU Compiler Collection](https://gcc.gnu.org)
- [GNU Make](https://www.gnu.org/software/make) or [Ninja](https://ninja-build.org) (recommended)
- [Doxygen](#doxygen) (only if generating documentation)

To build `libktx` such that the Basis Universal encoders will use
OpenCL you need

- OpenCL headers
- OpenCL driver

Additional requirements for the load tests applications

- SDL2 development library
- assimp development library
- OpenGL development libraries
- Vulkan development libraries
- [Vulkan SDK](#vulkan-sdk)
- zlib development library

On Ubuntu and Debian these can be installed via

```bash
sudo apt install build-essential cmake libzstd-dev ninja-build doxygen libsdl2-dev libgl1-mesa-glx libgl1-mesa-dev libvulkan1 libvulkan-dev libassimp-dev opencl-c-headers mesa-opencl-icd
```

`mesa-opencl-icd` should be replaced by the appropriate package for your GPU.


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
cmake . -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -D KTX_FEATURE_LOADTEST_APPS=ON -D KTX_FEATURE_DOC=ON

# Compile the project
cmake --build build
```

### Apple macOS/iOS

You need to install the following

- CMake
- Xcode
- [Doxygen](#doxygen) (only if generating documentation)

For the load tests applications you need to install the Vulkan SDK.
To build for iOS you need to set the CMake cache variable `MOLTEN_VK_SDK` to the root of MoltenVK inside the Vulkan SDK, if it is not already set.
Caution: `setup.env` in the macOS Vulkan SDK sets `VULKAN_SDK` to the macOS folder of the SDK, a sibling of the MoltenVK folder.
To build for other platforms, you shouldn't need to do anything else, but you might need to set the environment variable `VULKAN_SDK`
to the root of the Vulkan SDK as a hint for [FindVulkan](https://cmake.org/cmake/help/latest/module/FindVulkan.html#hints).

Other dependencies (like zstd, SDL2 or the assimp library are included in this repository or come with Xcode).

**NOTE:** the `iphoneos` or `MacOSX` SDK version gets hardwired into the generated projects. After installing an Xcode update that has the SDK for a new version of iOS, builds will fail. The only way to remedy this is to delete the build folder and regenerate from scratch.

#### macOS

To build for macOS:

```bash
# This creates an Xcode project at `build/mac/KTX-Software.xcodeproj` containing the libktx and tools targets.
mkdir build
cmake -G Xcode -B build/mac

# If you want to build the load test apps as well, you have to
# set the `KTX_FEATURE_LOADTEST_APPS` parameter:
cmake -GXcode -Bbuild/mac -D KTX_FEATURE_LOADTEST_APPS=ON

# Compile the project
cmake --build build/mac
```
##### Apple Silicon and Universal Binaries

Macs are either based on Intel or the newer Apple Silicon architecture. By default CMake configures to build for your host's platform, whichever it is. If you want to cross compile universal binaries (that support both platforms), add the parameter `-DCMAKE_OSX_ARCHITECTURES="\$(ARCHS_STANDARD)"` to cmake.

> **Known limitations:**
> - Intel Macs have support for SSE, but if you're building universal binaries,
>   you have to disable SSE or the build will fail

Example how to build universal binaries

```bash
# Configure universal binaries and disable SSE 
cmake -G Xcode -B build-macos-universal -D CMAKE_OSX_ARCHITECTURES="\$(ARCHS_STANDARD)" -D BASISU_SUPPORT_SSE=OFF
# Build 
cmake --build build-macos-universal
# Easy way to check if the resulting binaries are universal

file build-macos-universal/Debug/libktx.dylib
# outputs:
# build-macos-universal/Debug/libktx.dylib: Mach-O universal binary with 2 architectures: [x86_64:Mach-O 64-bit dynamically linked shared library x86_64] [arm64]
# build-macos-universal/Debug/libktx.dylib (for architecture x86_64):	Mach-O 64-bit dynamically linked shared library x86_64
# build-macos-universal/Debug/libktx.dylib (for architecture arm64):	Mach-O 64-bit dynamically linked shared library arm64

file build-macos-universal/Debug/toktx
# outputs:
# build-macos-universal/Debug/toktx: Mach-O universal binary with 2 architectures: [x86_64:Mach-O 64-bit executable x86_64] [arm64:Mach-O 64-bit executable arm64]
# build-macos-universal/Debug/toktx (for architecture x86_64):	Mach-O 64-bit executable x86_64
# build-macos-universal/Debug/toktx (for architecture arm64):	Mach-O 64-bit executable arm64
```

To explicity build for one or the other architecture use
`-D CMAKE_OSX_ARCHITECTURES=arm64` or `-D CMAKE_OSX_ARCHITECTURES=x86_64`

##### macOS signing

To sign the applications you need to set the following CMake variables:

| Name | Value |
| :--: | ----- |
| XCODE\_CODE\_SIGN\_IDENTITY | Owner* of the _Developer ID Application_ certificate to use for signing. |
| XCODE\_DEVELOPMENT\_TEAM | Development team of the certificate owner.

To sign the installation package you need to set the following variables:

| Name | Value |
| :--: | ----- |
| PRODUCTBUILD\_IDENTITY\_NAME | Owner* of the _Developer ID Installer_ certificate to use for signing. |
| PRODUCTBUILD\_KEYCHAIN\_PATH | Path to the keychain file with the certificate. Blank if its in the default keychain.

#### iOS

To build for iOS:

```bash
# This creates an Xcode project at `build/ios/KTX-Software.xcodeproj` containing the libktx targets.
mkdir build # if it does not exist
cmake -G Xcode -B build/ios -D CMAKE_SYSTEM_NAME=iOS

# This creates a project to build the load test apps as well.
cmake -G Xcode -B build/ios -D KTX_FEATURE_LOADTEST_APPS=ON"

# Compile the project
cmake --build build -- -sdk iphoneos
```

If using the CMake GUI, when it asks you to specify the generator for the project, you need to check _Specify options for cross-compiling_ and on the next screen make sure _Operating System_ is set to `iOS`.

##### iOS signing

To sign the applications you need to set the following CMake variables:

| Name  | Value |
| :---: | ----- |
| XCODE\_CODE\_SIGN\_IDENTITY | Owner* of the _Apple Development_ certificate to use for signing. |
| XCODE\_DEVELOPMENT\_TEAM | Development team used to create the Provisioning Profile. This may not be the same as the team of the _Apple Development_ certificate owner.
| XCODE\_PROVISIONING\_PROFILE | Name of the profile to use.

\* _Owner_ is what is formally known as the _Subject Name_ of a certificate. It
is the string displayed by the Keychain Access app in the list of installed
certificates and shown as the value of the _Common Name_ field of the _Subject
Name_ section of the details shown after double-clicking the certificate.

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
emcmake cmake -B build-web-debug . -D CMAKE_BUILD_TYPE=Debug

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

### Windows

CMake can create solutions for Microsoft Visual Studio (2015/2017/2019 are supported by KTX).

> **Note:** x86 (32-bit) Windows is not supported.

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

To configure for Universal Windows Platform (Windows Store) you have to

- Set the platform to `x64`, `ARM` or `ARM64` (depending on your target device/platform)
- Set the system name to `WindowsStore`
- Provide a system version (e.g. `10.0`)

> **Note:** Support is currently limited to `ktx` and `libktx_read` (no tools, tests or load tests apps)

Example UWP configuration

```bash
cmake . -A ARM64 -B build_uwp_arm64 -D CMAKE_SYSTEM_NAME:String=WindowsStore -D CMAKE_SYSTEM_VERSION:String="10.0"
# Build `ktx.dll` only
cmake -B build_uwp_arm64 --target ktx
```

A `bash` shell is needed by the `mkversion` script used during the build. If you installed your `git` via the
[Git for Windows](https://gitforwindows.org/) package you are good to go.
Alternatives are
[Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install) plus a Linux distribution or [Cygwin](https://www.cygwin.com/)  .

The NSIS compiler is needed if you intend to build packages.

CMake can include OpenGL ES versions of the KTX loader tests in the
generated solution. To build and run these you need to install an
OpenGL ES emulator. See [below](#opengl-es-emulator-for-windows).

The KTX loader tests use libSDL 2.0.12+. You do not need SDL if you only wish to build the library or tools.

The KTX vulkan loader tests require a [Vulkan SDK](#vulkan-sdk)
and the Open Asset Import Library [`libassimp`](#libassimp). You must
install the former. The latter is included in this repo.

##### Windows signing

To sign applications and the NSIS installer you need to import your certificate to either the Current User or Local Machine certificate store. This can be done
interactively with Windows' commands `certmgr` and `certlm` respectively. Then you need to set the following CMake variables:

| Name  | Value |
| :---: | ----- |
| WIN\_CODE\_SIGN\_IDENTITY | Owner* of the _Developer ID Application_ certificate to use for signing. |
| WIN\_CS\_CERT\_SEARCH\_MACHINE\_STORE | Check this option if your certificate is in the Local Machine store.

\* Owner is what is formally known as the _Subject Name_ of a certificate. It is
displayed in the _Issued To_ column of `certmgr` and `certlm`.

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

#### OpenCL for Windows

To build `libktx` such that the Basis Universal encoders will use
OpenCL you need an OpenCL driver, which is typically included in the driver for your GPU, and an OpenCL SDK. If no SDK is present, the build will use the headers and library that are included in this repo.

### Android

Support is currently limited to libktx and libktx_read (no tools, tests or loadtest apps)

Requirements:

- [CMake](https://cmake.org)
- [Android NDK](https://developer.android.com/ndk)

The path to the NDK, a CMake toolchain file (that comes with the NDK), the desired Android ABI and minimum API level have to be provided when configuring with CMake (see [Android NDK CMake guide](https://developer.android.com/ndk/guides/cmake) for more details/settings). Example:

```bash
export ANDROID_NDK=/path/to/Android_NDK #This is the location of Android NDK
# Configure
cmake . -B "build-android" \
-DANDROID_PLATFORM=android-24 \ # API level 24 equals Android 7.0
-DANDROID_ABI="arm64-v8a" \ # target platform
-DANDROID_NDK="$ANDROID_NDK" \
-DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \ # Toolchain file in a subfolder of the NDK
-DBASISU_SUPPORT_SSE=OFF # Disable SSE

# Build
cmake --build "build-android"
```

> Note: SSE has to be disabled currently (for ABIs x86 and x86_64) due to [an issue](https://github.com/BinomialLLC/basis_universal/pull/233).

Dependencies
------------

### SDL

Needed if you want to build the KTX load tests.

On GNU/Linux you need to install `libsdl2-dev` using your package manager. Builds
of SDL are provided in the KTX Git repo for iOS, macOS and Windows. These
binaries were built from the 2.0.20 tag. For macOS and Windows you can download
binaries from [libsdl.org](https://www.libsdl.org/download-2.0.php), if you
prefer.

#### macOS Notes

To build for both Intel and Apple Silicon you need a universal binary
build of SDL as is provided in the KTX Git repo.

For Apple Silicon you need at least release 2.0.14 of SDL.

#### Building SDL from source

As noted above, KTX uses
[SDL release 2.0.20](https://github.com/libsdl-org/SDL/tree/release-2.0.20) in
the canonical Mercurial repo at https://github.com/libsdl-org/SDL. Clone the repo, checkout tag `release-2.0.20`and follow the SDL build instructions.

Copy the results of your build to the appropriate place under the
`other_lib` directory.

### Vulkan SDK

Needed if you want to build the KTX Vulkan load tests, `vkloadtests`.

Download the [Vulkan SDK from Lunar G](https://vulkan.lunarg.com/sdk/home).

For Ubuntu (Xenial and Bionic) install packages are available. See [Getting
Started - Ubuntu](https://vulkan.lunarg.com/doc/sdk/1.2.141.2/linux/getting_started_ubuntu.html) for detailed instructions.

For other GNU/Linux distributions a `.tar.gz` file is available. See
[Getting Started - Tarball](https://vulkan.lunarg.com/doc/sdk/1.2.141.2/linux/getting_started.html) for detailed instructions.

For Windows install the Vulkan SDK via the installer.

For iOS and macOS, install the Vulkan SDK by downloading the macOS installer and double-clicking _install_ in the mounted `.dmg`. You need version 1.2.189.1 or later for Apple Silicon support. This SDK contains MoltenVK (Vulkan Portability on Metal) for both iOS and macOS.

### Doxygen

Needed if you want to generate the _libktx_ and _ktxtools_ documentation.

You need a minimum of version 1.8.14 to generate the documentation correctly. You
can download binaries and also find instructions for building it from source at
[Doxygen downloads](http://www.stack.nl/~dimitri/doxygen/download.html). Make
sure the directory containing the `doxygen` executable is in your `$PATH`.

### libassimp

Needed if you want to build the KTX load tests.

On GNU/Linux you need to install the Open Asset Import Library [`libassimp-de`]
using your package manager. The KTX Git repo has binaries for iOS, macOS and Windows.

Canonical source is at https://github.com/assimp/assimp. 

### OpenCL

Needed if you want to enable the Basis Universal encoders to use OpenCL when
building _libktx_.

On GNU/Linux and Windows you need to install an OpenCL SDK and OpenCL driver. Drivers are standard on macOS & iOS and Xcode includes the SDK. On GNU/Linux the SDK can be installed using your package manager. On Windows, the place from which to download the SDK depends on your GPU vendor. In both cases, the GPU driver typically includes an OpenCL driver. 


{# vim: set ai ts=4 sts=4 sw=2 expandtab textwidth=75:}
