<!-- Copyright 2026, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 5.0.0-rc1

### Summary

This is a major release. Although the reason for bumping the major number is a small but incompatible change in the *libktx* API, there are major changes throughout the software:

* __`ktxBasisParams` API has two incompatible changes.__
* Support for the Binomial UASTC HDR formats has been added to all supported API bindings and relevant tools.
* The *ktx* tool suite is now a superset of the legacy tools.
* __The legacy tools have been removed.__
* A CMake project has been added to build *libktx*.
* Source code of dependencies is no longer included in CMake target source lists. All dependencies are now built by their own CMake projects and linked to *libktx*.

This is a pre-release because the version of the KTX specification with which it is compliant, Document Revision 5, has not yet been published. The source of the draft is available at https://github.com/KhronosGroup/KTX-Specification/pull/216.

### New Features in v5.0
#### Tools

* Images can be encoded to the Binomial *UASTC HDR 4x4* and *UASTC HDR 6x6 Intermediate* formats in __*ktx create*__ and __*ktx encode*__. Images in these formats can be transcoded to ASTC HDR or BC6H Unsigned formats.
* __*ktx extract*__ can extract images in ASTC HDR formats from KTX v2 files. It writes them to  `.exr` files. It will apply `KTXmapRange` during extraction.
* __*ktx create*__ accepts a `--premultiply-alpha` option.
* __*ktx create*__ can resize or scale its input files, except `.raw`.
* __*ktx create*__ accepts file name listing files as input. Prefix the name of such a file with `@` or `@@`.
* __*ktx convert*__ for converting KTX v1 files.

This shows the equivalent new tool for each removed legacy tool.

| Legacy Tool | `ktx` tool |
| :---------: | :--------: |
| ktx2ktx2 | convert -t ktx |
| ktx2check | validate |
| ktxinfo | info |
| ktxsc | deflate or encode |
| toktx | create |

 
#### *libktx* functions

* `ktxTexture2_CompressBasisEx` can  now encode to Binomial *UASTC HDR 4x4* and *UASTC HDR 6x6 Intermediate* formats. There are two incompatible changes in the struct `ktxBasisParams` passed to this function:
    * `ktx_bool_t uastc` is now `ktx_basis_codec_e codec` which accepts the following values
        * `KTX_BASIS_CODEC_NONE`
        * `KTX_BASIS_CODEC_ETC1S`
        * `KTX_BASIS_CODEC_UASTC_LDR_4x4`
        * `KTX_BASIS_CODEC_UASTC_HDR_4x4`
        * `KTX_BASIS_CODEC_UASTC_HDR_6x6_INTERMEDIATE`
    * `compressionLevel` is now named `etc1sCompressionLevel`
* `ktxTexture2_DecodeAstc` can now decode ASTC HDR files.
* `ktxTexture2_TranscodeBasis` can transcode images in *UASTC HDR 4x4* and *UASTC HDR 6x6 Intermediate* formats to ASTC HDR or BC6H Unsigned.
  
The new functionality is exposed in the Java, JavaScript and Python bindings.

### Notable Fixes in v5.0

* A crash in ___ktx create___ when reading tiled `.exr` files has been fixed.
* An extra swizzle, when generating mip levels when the output image components are not in RGBA order, leading to incorrect colors in some mip levels has been
fixed.
* The default wrap mode when generating mip levels in __*ktx create*__ has been changed to `CLAMP`, the same default that was set in __*toktx*__.
* When loading `.exr` files in `ktx create`, the default primaries are set to `BT709` instead of `UNSPECIFIED`.
* Non-sRGB ASTC format KTX v1 files are mapped to the `VK_FORMAT_ASTC_*_SFLOAT_BLOCK` format with the same block dimensions when uploaded to Vulkan or converted to KTX v2 instead of the `VK_FORMAT_ASTC_*_UNORM_BLOCK` format.
* Per a bug fix in the Khronos Data Format Specification, the Data Format Descriptor's `channelType` field is now called `channelId`. This change is visible in the outputs of __*ktx info*__ and __*ktx compare*__. New schema versions have been published for their JSON outputs.

### Known Issues

* Files deflated with zlib using *libktx* compiled with GCC and run on x86\_64 may not be bit-identical with those using *libktx* compiled with GCC and run on arm64.

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using __*ktx create*__, or use the `--resize` feature to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of __*ktx validate*__.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In __*ktx create*__ or  __*ktx encode*__ use `--threads 1` for the former or `--uastc_rdo_m` for the latter. Results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.

### Acknowledgements

Thanks to all who have contributed to this release. Their GitHub handles are included in the list of commits below. Particluar thanks to:

* Rich Geldreich (@richgel999) for creating the transcodable HDR technology.
* Phasmatic (@ViNeek and @agkar) who did the bulk of the HDR-related work under
  contract from Khronos.
* Google for funding that work.
* Marco Hutter (@javagl) for adding the HDR api to the Java binding.
* @jiangzhhhh for adding pre-multiplied alpha support to __*ktx create*__.

### Commits since v4.4.2 (by part with details)

Note: commits will appear in each part they affected.

### libktx

* Bump default version number. (#1161) (6c474d862) (@MarkCallow)

  Various other issues were encountered and fixed while checking the
  version number related changes.

  1. Fix `-a` option of `mkversion`. It did not visit `lib/src` and ended
     with an error due to a misplaced `fi`.
  2. Fix RE in _ktx convert_ that resulted in an extra space in an edited
     KTXwriterScParams when the original `zcmp` option had a parameter.
  3. Fix `ktxTexture2_IsHDR` which was only checking for a FLOAT qualifier
     on DFD samples for the ASTC color model. Add a test for it.

  Fixes #1112.

* Fix build issues (#1160) (f2d222d8a) (@MarkCallow)

  1. Suppress what looks to be a bogus stringop-overflow warning from GCC,
     observed with both versions 13 and 15.
  2. Change outdated SSE and OPENCL option names to current names so they
     are acted on properly.

  Given the issues being fixed here, I am unable to understand why our CI
  builds have been green.

* Remove `--rec2020` flag and add range mapping functionality to `ktx create` and `ktx extract` (#1157) (20211a217) (@ViNeek)

  This PR fixes #1142 and fixes #1127.

  For #1127, it removes the flag from `ktx create` and `ktx encode` and
  makes sure that the profile is automatically inferred from the loaded
  EXR file.

  For #1142 It adds 3 knew flags to `ktx create`, namely,
  `map-range-auto`, `map-range-offset` and `map-range-scale` to be used
  for mapping of floating point values in HDR formats.
   
  `ktx extract` is also updated to handle inverse mapping of values when
  the `KTXmapRange` key/value pair is present in a KTX2 file.

  Currenlty, `offset` and `scale` are not used for the alpha channel.
  Currently, range mapping only works with floating point data types and
  the new HDR formats.

* Proper handling of --uastc-quality and --uastc-hdr-6x6i-level option values  (#1149) (de9a41a32) (@ViNeek)

  This PR fixes #1146. The values are properly forwarded to the basis
  encoder.

  Includes fix to make ktxdiff properly compare half-float files.
  
* Ensure enums with ktx\_uint32\_t equivalents really are unsigned. (#1152) (23f560d40) (@MarkCallow)

* Update basisu for fixes backported from 2.0 (#1151) (43c716c5e) (@MarkCallow)

  These are:
  - Critical fixes for extreme inputs/outputs
  - Compiler warning
  - Fix parsing of uint64 in KTX2 header
  - Fix initialization of astc\_helper tables for transcoding.

  Remove our workaround for the astc\_helper tables not being initialized.

  Fix ktxdiff to choose correct transcode target when comparing HDR
  payloads.

* Expose HDR support in Python binding (#1150) (5444378e1) (@MarkCallow)

  Fixes #1145.

  Additional fixes:
  - Adds `num_layers` property that was curiously missing.
  - Exposes ktxTexture[12]_IsHDR in the c API.
  - Reformats documentation comments for UASTC HDR ktxBasisParams to
     reduce line width.

* Fix dumping of encoder input data (7ad1889e6) (@MarkCallow)

  for compilers supporting std::filesystem and for HDR.

* Fix validation warning for VK\_IMAGE\_LAYOUT\_SHADER\_READ\_ONLY\_OPTIMAL c… (#1148) (2914abe3a) (@MarkCallow)

  …ase.

  For the `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` case of
  `setImageLayout`, where `dstAccessMask` is `VK_ACCESS_SHADER_READ_BIT`,
  setting`destStageFlags` to `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT` is
  invalid usage and is flagged by the validator in recent Vulkan SDKs.
  Change `dstStageFlags` for this case to
  `VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT`.

  Fixes #1092.

  Includes completely unrelated fixes for warnings that are new in
  Emscripten 5.0.3 so that CI builds are successful.
  - Set -Wno-experimental to disable compile- and link-time warnings that
  sdl3 is still experimental.
  - Change `EMSCRIPTEN` pre-processor macro to `__EMSCRIPTEN__` to stop
  deprecation warnings.

* Document that GL\_COMPRESSED\_RGBA\_ASTC formats are converted to SFLOAT equivalents. (1499a5534) (@MarkCallow)

* Enable decode and extraction of ASTC HDR. (#1143) (af3039c62) (@MarkCallow)

  Fixes #1140.

* Remove dead code. (535c77df6) (@MarkCallow)

* CMake improvements for installing and dependencies (#1133) (8b925f225) (@MarkCallow)

  * Handle installation of `KHR/khr_df.h` for KTX-Software in KTX-Software
    (`lib/CMakeLists.txt`) avoiding some confusion and making installation
    in a framework possible.
  * Use add\_lib\_dependency macro to add dfdutils dependency to libktx and
    remove unnecessary PUBLIC export.
  * Use FILE\_SETs for installation of both dfdutils and libktx include
    files to handle the KHR subdirectory, with a workaround for framework
    installation.
  * Up the cmake minimum version to 3.23 in both dfdutils CMakeLists and
    lib CMakeLists.txt This is so
      - FILE\_SET can be used
      - KTX-Software can override options settings with ordinary variables
        thus hiding unnecessary DFDUTILS options from the CMake GUI.
  * Add option to turn off dfdutils docs build so `lib/CMakeLists.txt` can
    do so.
  * Update dfdutils install to install library and `dfd.h` in addition to
    `KHR/khr_df.h`. This is behind an option so the KTX-Software build can
    turn it off.
  * Include GNUInstallDirs to get proper values for CMAKE\_INSTALL\_<dir>.
  * Normalize the indentation in dfdutils/CMakeLists.txt.
  * Improve command clarity.

  `cmake --build ... --target install` and the installable packages have
  been tested on all 3 desktop platforms and the framework in the iOS .zip
  file and content of the Android .zip file have been verified. All have
  the expected content.

  Fixes #1121. Fixes #1123.

* Fix issue with HDR transcode and extract (#1139) (5d29c574a) (@ViNeek)

  The PR should fix the issues with `ktx extract` for both UASTC 4x4 and
  UASTC 6x6i (see issue #1134). The code has also been patched to properly
  extract mip map levels. Thank you @richgel999 for providing the code
  snippet.

  Extraction from regular ASTC data `ktx extract --all astc_hdr_6x6.ktx2`
  won't work. Direct encoding/decoding of VK\_FORMAT\_ASTC\_6x6\_SFLOAT\_BLOCK
  and VK\_FORMAT\_ASTC\_4x4\_SFLOAT\_BLOCK (or any other SFLOAT block size) is
  not supported by the current version of the software. Same goes with
  `ktx create`:

  A command like `ktx create --format VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK
  input.exr output.ktx2` does not support the creation of an
  `VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK`-encoded ktx2 file.

  Fixes #1134.

* Add HDR support to vkloadtests (#1130) (dcc434604) (@MarkCallow)

  Invoke with `--hdr` to get an R16G16B16 HDR rendering surface.

  * Add support for transcoding the new HDR formats.
  * Add a sample in each of UASTC HDR 4x4 and UASTC HDR 6x6i.

* Improved HDR support (#1124) (35d9f8d53) (@ViNeek)

  Fixes #1122, #1119, #1120 and #1125.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Remove legacy tools (#1110) (05b0e0289) (@MarkCallow)

  - Remove legacy tools, documentation and references to them.
  - Reorganize and normalize the names of still needed test images (now
    under tests/resources).
  - Rewrite genktx2 script to generate needed .ktx2 resources using `ktx`
    suite.
  - Fix missing features and bugs in suite discovered when rewriting
    genktx2:
    - Fix default imageio scanline reader to only buffer and rescale a
      scanline's worth of pixels.
    - Add channel add/remove functionality to default scanline reader
      as `ktx create` always requests 4 channels.
    - Fix ImageT::yflip to not attempt to yflip a 1 row image thus not
      crash.
    - Add listing file support to `ktx create` so multiple input files names
      can be provided as a list in another file
    - Fix so sample qualifierLinear does not cause sameUnitAllChannels not
      to be set.
  - Fix unit tests to compile with c++20 and simplify file handling by
    using std::filesystem.
    - texturetests, transcodetests, streamtests & threadtests now use
      std::format. This PR works around GCC 11's (the GCC version on
      Ubuntu 22.04 CI runners) lack of support for std::format by using
      fmt::format in that case.

* Add lock around transcoder initialization (#1098) (964acdc17) (@MarkCallow)

  Fixes #1087.

  Add threadtests - though unsure how useful they are.

  Update all libktx tests for c++20 as new tests need it for std::barrier.

  Fixes basisu\_c\_binding build when ktx\_read target is not included in
  build.

  Big thanks to @vmwalker for reporting the problem and providing the fix.

* Combine lib dependencies into static libktx on all desktop platforms (#1090) (5a07bc6f8) (@MarkCallow)

  Previously this was only done on macOS - using `libtool`. Changed to
  a simple, though hard to find, cross-platform way to do this via CMake.

  Add CI test of build and use of a static library.

  Remove macOS 13 build from CI as these runner images have been
  retired from GitHub Actions.

* Fix use of libktx project as subproject outside of KTX-Software (#1089) (7f0f889e3) (@MarkCallow)

  #### The Fix

  * Always include `cmake/codesign.cmake` and `cmake/cputypetest.cmake`.
    Add `include_guard()` to them to prevent multiple inclusion.
  * Add host project for testing sub-project use and update workflow
     to build and run it and libktx.

  #### Other Changes
  * Add options to select building of full or read-only libktx.
  * Fix macOS build when CODE\_SIGN\_IDENTITY not set. Fixes
    both libktx and the larger KTX-Software project.

  Fixes #1083

* Fix creating static ktx\_read convenience lib (#1081) (4b46cda2a) (@dg0yt)

  Fixes "duplicate member name" warnings:
  ~~~
  [130/148] : [...] libtool -static -o
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx\_read.a
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx.a
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/external/astc-encoder/Source/libastcenc-avx2-static.a

  /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/libtool:
  warning for architecture: x86\_64h duplicate member name
  'astcenc\_averages\_and\_directions.cpp.o' from '/Users/vcpkg/Data/b/ktx/x64-osx-rel/external/astc-encoder/Source/libastcenc-avx2-static.a(astcenc\_averages\_and\_directions.cpp.o)'
  and
  '/Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx.a(astcenc\_averages\_and\_directions.cpp.o)'
  ~~~

* Update basis\_universal (#1080) (422f501ea) (@MarkCallow)

  master, in the fork from which we pull basis\_universal, has been updated
  to basis\_universal master as of 2025.11.22, which includes all our
  previous warning fixes. The changes there have been merged to the
  `cmake_fixes` branch we pull from.

  Also included:
  * fix: change basis-related cmake option names to match those in
  basis\_universal and update workflows to set the new names;
  * fix: deploy x86\_64 package, broken when build moved to x86\_64 runner;
  * enhancement: apply changes resulting from a review of the cmake\_fixes
  PR for basis\_universal.

  git subrepo clone --branch=cmake\_fixes --force https://github.com/KhronosGroup/basis\_universal.git external/basis\_universal

  subrepo:
    subdir:   "external/basis\_universal"
    merged:   "daf79c6ee"
  upstream:
    origin:   "https://github.com/KhronosGroup/basis\_universal.git"
    branch:   "cmake\_fixes"
    commit:   "daf79c6ee"
  git-subrepo:
    version:  "0.4.9"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "4f60dd7"

  Fixes #1079.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

* Create libktx project (#1071) (6765255be) (@MarkCallow)

  Move the libktx build from the root CMakeLists.txt to its own in the
  `lib` directory. Although still within KTX-Software, the new libktx
  project `CMakeLists.txt` can be used as either a sub-project of
  KTX-Software or as a standalone project to build just libktx.

  This has been done due to both direct requests, such as
  https://github.com/KhronosGroup/KTX-Software/discussions/995 and
  indirect requests such as complaints that the root CMakeLists is too
  complicated. The effort has led to some useful cleanup and better
  modularization.

  The following FEATURE options have been moved to the libktx project and
  consequently renamed with a `LIBKTX` prefix:
  - LIBKTX\_FEATURE\_KTX1 
  - LIBKTX\_FEATURE\_KTX2
  - LIBKTX\_FEATURE\_VK\_UPLOAD
  - LIBKTX\_FEATURE\_GL\_UPLOAD
  - LIBKTX\_FEATURE\_ETC\_UNPACK

  These options have been duplicated in the libktx project so they can be
  set when it is the top-level project:
  - LIBKTX\_EMBED\_BITCODE
  - LIBKTX\_WERROR

  When libktx is used as a sub-project, they are set from the similarly
  named KTX-Software options.

  Add a workflow to test configuring and building a standalone libktx.

  Along the way, fixed a bug, that caused configurations in the linux.yml
  workflow that were supposed to incorporate OpenCL to not do so. However
  it is not possible to run the tests on those configurations as the
  Actions runners report the cpu type as "generic" so Portable OpenCL
  (POCL) can't figure out what to do.

  Also along the way, update the ignore lists in all the workflow files to
  include newer workflows files.

* Fix potential buffer read overflow in ktxTexture2\_WriteToStream. (14bc6f0c7) (@MarkCallow)

  This was caused on Android (Linux also?) by `memchr` trying to read `n` bytes where `n` is the guard length passed to it before searching for the wanted character. Fix is to avoid `memchr` and any other external function in the homegrown `strnlen` used on Android, Linux, Windows and the web.

  Fixes #1064.

* ktx create: add premultiply alpha option (#1049) (83d84dce6) (@jiangzhhhh)

  * Add alpha pre-multiply function to image.hpp.

  * Update base Texture load test to check for pre-multiplied textures
    and add a sample to the tests.

### Tools

* Bump default version number. (#1161) (6c474d862) (@MarkCallow)

  Various other issues were encountered and fixed while checking the
  version number related changes.

  1. Fix `-a` option of `mkversion`. It did not visit `lib/src` and ended
     with an error due to a misplaced `fi`.
  2. Fix RE in _ktx convert_ that resulted in an extra space in an edited
     KTXwriterScParams when the original `zcmp` option had a parameter.
  3. Fix `ktxTexture2_IsHDR` which was only checking for a FLOAT qualifier
     on DFD samples for the ASTC color model. Add a test for it.

  Fixes #1112.

* Fix build issues (#1160) (f2d222d8a) (@MarkCallow)

  1. Suppress what looks to be a bogus stringop-overflow warning from GCC,
     observed with both versions 13 and 15.
  2. Change outdated SSE and OPENCL option names to current names so they
     are acted on properly.

  Given the issues being fixed here, I am unable to understand why our CI
  builds have been green.

* Fix handling of DFD linear qualifier (#1158) (3148f7281) (@MarkCallow)

  Fix `ktx create` to set the linear qualifier according to the transfer
  function and channel id, instead of always setting it to the default
  value for the output VkFormat.

  Fix the validator to allow values that differ from the default provided
  they comply with the KTX specification.

  Fixes #1156.

* Change channelType to channelId to match latest KDFS. (#1155) (57ef740fe) (@MarkCallow)

* Remove `--rec2020` flag and add range mapping functionality to `ktx create` and `ktx extract` (#1157) (20211a217) (@ViNeek)

  This PR fixes #1142 and fixes #1127.

  For #1127, it removes the flag from `ktx create` and `ktx encode` and
  makes sure that the profile is automatically inferred from the loaded
  EXR file.

  For #1142 It adds 3 knew flags to `ktx create`, namely,
  `map-range-auto`, `map-range-offset` and `map-range-scale` to be used
  for mapping of floating point values in HDR formats.
   
  `ktx extract` is also updated to handle inverse mapping of values when
  the `KTXmapRange` key/value pair is present in a KTX2 file.

  Currenlty, `offset` and `scale` are not used for the alpha channel.
  Currently, range mapping only works with floating point data types and
  the new HDR formats.

* Fix typo in comment. (bc8ec6cd1) (@MarkCallow)

* Reinstate ktxtools\_mainpage. (4ffa3f0d0) (@MarkCallow)

  Fix issues in its content.

* Document that GL\_COMPRESSED\_RGBA\_ASTC formats are converted to SFLOAT equivalents. (1499a5534) (@MarkCallow)

* Enable decode and extraction of ASTC HDR. (#1143) (af3039c62) (@MarkCallow)

  Fixes #1140.

* CMake improvements for installing and dependencies (#1133) (8b925f225) (@MarkCallow)

  * Handle installation of `KHR/khr_df.h` for KTX-Software in KTX-Software
    (`lib/CMakeLists.txt`) avoiding some confusion and making installation
    in a framework possible.
  * Use add\_lib\_dependency macro to add dfdutils dependency to libktx and
    remove unnecessary PUBLIC export.
  * Use FILE\_SETs for installation of both dfdutils and libktx include
    files to handle the KHR subdirectory, with a workaround for framework
    installation.
  * Up the cmake minimum version to 3.23 in both dfdutils CMakeLists and
    lib CMakeLists.txt This is so
      - FILE\_SET can be used
      - KTX-Software can override options settings with ordinary variables
        thus hiding unnecessary DFDUTILS options from the CMake GUI.
  * Add option to turn off dfdutils docs build so `lib/CMakeLists.txt` can
    do so.
  * Update dfdutils install to install library and `dfd.h` in addition to
    `KHR/khr_df.h`. This is behind an option so the KTX-Software build can
    turn it off.
  * Include GNUInstallDirs to get proper values for CMAKE\_INSTALL\_<dir>.
  * Normalize the indentation in dfdutils/CMakeLists.txt.
  * Improve command clarity.

  `cmake --build ... --target install` and the installable packages have
  been tested on all 3 desktop platforms and the framework in the iOS .zip
  file and content of the Android .zip file have been verified. All have
  the expected content.

  Fixes #1121. Fixes #1123.

* Fix issue with HDR transcode and extract (#1139) (5d29c574a) (@ViNeek)

  The PR should fix the issues with `ktx extract` for both UASTC 4x4 and
  UASTC 6x6i (see issue #1134). The code has also been patched to properly
  extract mip map levels. Thank you @richgel999 for providing the code
  snippet.

  Extraction from regular ASTC data `ktx extract --all astc_hdr_6x6.ktx2`
  won't work. Direct encoding/decoding of VK\_FORMAT\_ASTC\_6x6\_SFLOAT\_BLOCK
  and VK\_FORMAT\_ASTC\_4x4\_SFLOAT\_BLOCK (or any other SFLOAT block size) is
  not supported by the current version of the software. Same goes with
  `ktx create`:

  A command like `ktx create --format VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK
  input.exr output.ktx2` does not support the creation of an
  `VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK`-encoded ktx2 file.

  Fixes #1134.

* Fix: stop extra swizzle when generating mipmaps. (#1137) (29a149cc7) (@MarkCallow)

  And generate all mip levels from the base level instead of
  the previous level.

  Fixes #1116.

* Change default wrap mode to CLAMP. Remove deprecated oetf options. (dcd0e939c) (@MarkCallow)

  Fixes #1118.

* Support loading tiled EXR files. (#1135) (3abe70a60) (@MarkCallow)

  Add face order info to --cubemap info in command\_create.

  Fixes issue #1111.

* Change default primaries for EXR (#1128) (f1c169df4) (@MarkCallow)

  from UNSPECIFIED to BT709, per the EXR spec.

* Improved HDR support (#1124) (35d9f8d53) (@ViNeek)

  Fixes #1122, #1119, #1120 and #1125.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Fix build problems w/ fmt and C++23 (#1102) (b3619b25b) (@ltjax)

  Fixes #1101 .

* Remove legacy tools (#1110) (05b0e0289) (@MarkCallow)

  - Remove legacy tools, documentation and references to them.
  - Reorganize and normalize the names of still needed test images (now
    under tests/resources).
  - Rewrite genktx2 script to generate needed .ktx2 resources using `ktx`
    suite.
  - Fix missing features and bugs in suite discovered when rewriting
    genktx2:
    - Fix default imageio scanline reader to only buffer and rescale a
      scanline's worth of pixels.
    - Add channel add/remove functionality to default scanline reader
      as `ktx create` always requests 4 channels.
    - Fix ImageT::yflip to not attempt to yflip a 1 row image thus not
      crash.
    - Add listing file support to `ktx create` so multiple input files names
      can be provided as a list in another file
    - Fix so sample qualifierLinear does not cause sameUnitAllChannels not
      to be set.
  - Fix unit tests to compile with c++20 and simplify file handling by
    using std::filesystem.
    - texturetests, transcodetests, streamtests & threadtests now use
      std::format. This PR works around GCC 11's (the GCC version on
      Ubuntu 22.04 CI runners) lack of support for std::format by using
      fmt::format in that case.

* Fix C++20 build issues. (#1104) (9d8594b24) (@MarkCallow)

  - Remove OutputString:: prefix from in class definition of constructor.
  - Add u8string overload of InputStream constructor.
  - Fix return type of from\_u8string.
  - Add from\_u8string and to\_u8string overloads that take u8string\_view
     and string\_view respectively.
  - Provide overloads of fmtInFile and fmtOutFile that take u8string\_view.
  - Avoid use of filesystem::path::u8path in c++20 to avoid deprecation
     warning.

* Support listing files for multiple inputs and u8string utf8 utilities. (#1103) (bb7c7d100) (@MarkCallow)

  This is a piece of legacy tool functionality, used in our asset
  generation script, that was missing.

  This PR also adds a u8string version of the utf8 utilities in
  platform\_utils.h and u8string constructors to OutputStream{,Ex} to make
  things work when building with c++20 and newer. All is protected by
  macro checks for u8string support.

* Add convert command (#1099) (0eaebb015) (@MarkCallow)

  For converting ktx v1 files.

  Fixes formatting error in encode man page (doxygen).

* Add lock around transcoder initialization (#1098) (964acdc17) (@MarkCallow)

  Fixes #1087.

  Add threadtests - though unsure how useful they are.

  Update all libktx tests for c++20 as new tests need it for std::barrier.

  Fixes basisu\_c\_binding build when ktx\_read target is not included in
  build.

  Big thanks to @vmwalker for reporting the problem and providing the fix.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

* Create libktx project (#1071) (6765255be) (@MarkCallow)

  Move the libktx build from the root CMakeLists.txt to its own in the
  `lib` directory. Although still within KTX-Software, the new libktx
  project `CMakeLists.txt` can be used as either a sub-project of
  KTX-Software or as a standalone project to build just libktx.

  This has been done due to both direct requests, such as
  https://github.com/KhronosGroup/KTX-Software/discussions/995 and
  indirect requests such as complaints that the root CMakeLists is too
  complicated. The effort has led to some useful cleanup and better
  modularization.

  The following FEATURE options have been moved to the libktx project and
  consequently renamed with a `LIBKTX` prefix:
  - LIBKTX\_FEATURE\_KTX1 
  - LIBKTX\_FEATURE\_KTX2
  - LIBKTX\_FEATURE\_VK\_UPLOAD
  - LIBKTX\_FEATURE\_GL\_UPLOAD
  - LIBKTX\_FEATURE\_ETC\_UNPACK

  These options have been duplicated in the libktx project so they can be
  set when it is the top-level project:
  - LIBKTX\_EMBED\_BITCODE
  - LIBKTX\_WERROR

  When libktx is used as a sub-project, they are set from the similarly
  named KTX-Software options.

  Add a workflow to test configuring and building a standalone libktx.

  Along the way, fixed a bug, that caused configurations in the linux.yml
  workflow that were supposed to incorporate OpenCL to not do so. However
  it is not possible to run the tests on those configurations as the
  Actions runners report the cpu type as "generic" so Portable OpenCL
  (POCL) can't figure out what to do.

  Also along the way, update the ignore lists in all the workflow files to
  include newer workflows files.

* ktx create: add premultiply alpha option (#1049) (83d84dce6) (@jiangzhhhh)

  * Add alpha pre-multiply function to image.hpp.

  * Update base Texture load test to check for pre-multiplied textures
    and add a sample to the tests.

### JS Bindings

* Remove `--rec2020` flag and add range mapping functionality to `ktx create` and `ktx extract` (#1157) (20211a217) (@ViNeek)

  This PR fixes #1142 and fixes #1127.

  For #1127, it removes the flag from `ktx create` and `ktx encode` and
  makes sure that the profile is automatically inferred from the loaded
  EXR file.

  For #1142 It adds 3 knew flags to `ktx create`, namely,
  `map-range-auto`, `map-range-offset` and `map-range-scale` to be used
  for mapping of floating point values in HDR formats.
   
  `ktx extract` is also updated to handle inverse mapping of values when
  the `KTXmapRange` key/value pair is present in a KTX2 file.

  Currenlty, `offset` and `scale` are not used for the alpha channel.
  Currently, range mapping only works with floating point data types and
  the new HDR formats.

* Improved HDR support (#1124) (35d9f8d53) (@ViNeek)

  Fixes #1122, #1119, #1120 and #1125.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

### Java Bindings

* Update Java bindings for HDR support (#1147) (459c0a98a) (@javagl)

  The PR https://github.com/KhronosGroup/KTX-Software/pull/1100 added HDR
  support.

  This PR updates the Java bindings accordingly so fixes #1144.

  What it does:

  - Adds the `UASTC_HDR_6X6_INTERMEDIATE` supercompression scheme enum
  value
  - Adds the `RGBA_HALF`/`ASTC_HDR_4x4_RGBA`/`ASTC_HDR_6x6_RGBA` transcode
  format enum values
  - Adds a new `KtxBasisCodec` class, corresponding to the codec enum, as
  requested in [a review
  comment](https://github.com/KhronosGroup/KTX-Software/pull/1100#discussion\_r2725412580)
  - Adds the new fields that have been added to `ktxBasisParams` (like
  `uastcHDRUltraQuant` etc) to the `KtxBasisParams` class
  - (Minor corresponding cleanups)
  - Wires the new fields to the native side accordingly, e.g.
  [initialize](https://github.com/KhronosGroup/KTX-Software/blob/1499a5534b19afd7cd5cf1d5e5c1296ce36d1f14/interface/java\_binding/src/main/cpp/libktx-jni.cpp#L176)
  and [fills the native
  struct](https://github.com/KhronosGroup/KTX-Software/blob/1499a5534b19afd7cd5cf1d5e5c1296ce36d1f14/interface/java\_binding/src/main/cpp/libktx-jni.cpp#L322)
  - Adds native versions of the 
  `KTX_API ktx_bool_t KTX_APIENTRY ktxTexture1_IsTranscodable(ktxTexture1*
  This);`
    functions (for `KtxTexture1` and `KtxTexture2`)
  - Adds `getColorModel`, `getTransferFunction`, `isHDR`.
  - Adds a couple of tests.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Remove legacy tools (#1110) (05b0e0289) (@MarkCallow)

  - Remove legacy tools, documentation and references to them.
  - Reorganize and normalize the names of still needed test images (now
    under tests/resources).
  - Rewrite genktx2 script to generate needed .ktx2 resources using `ktx`
    suite.
  - Fix missing features and bugs in suite discovered when rewriting
    genktx2:
    - Fix default imageio scanline reader to only buffer and rescale a
      scanline's worth of pixels.
    - Add channel add/remove functionality to default scanline reader
      as `ktx create` always requests 4 channels.
    - Fix ImageT::yflip to not attempt to yflip a 1 row image thus not
      crash.
    - Add listing file support to `ktx create` so multiple input files names
      can be provided as a list in another file
    - Fix so sample qualifierLinear does not cause sameUnitAllChannels not
      to be set.
  - Fix unit tests to compile with c++20 and simplify file handling by
    using std::filesystem.
    - texturetests, transcodetests, streamtests & threadtests now use
      std::format. This PR works around GCC 11's (the GCC version on
      Ubuntu 22.04 CI runners) lack of support for std::format by using
      fmt::format in that case.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

* Create libktx project (#1071) (6765255be) (@MarkCallow)

  Move the libktx build from the root CMakeLists.txt to its own in the
  `lib` directory. Although still within KTX-Software, the new libktx
  project `CMakeLists.txt` can be used as either a sub-project of
  KTX-Software or as a standalone project to build just libktx.

  This has been done due to both direct requests, such as
  https://github.com/KhronosGroup/KTX-Software/discussions/995 and
  indirect requests such as complaints that the root CMakeLists is too
  complicated. The effort has led to some useful cleanup and better
  modularization.

  The following FEATURE options have been moved to the libktx project and
  consequently renamed with a `LIBKTX` prefix:
  - LIBKTX\_FEATURE\_KTX1 
  - LIBKTX\_FEATURE\_KTX2
  - LIBKTX\_FEATURE\_VK\_UPLOAD
  - LIBKTX\_FEATURE\_GL\_UPLOAD
  - LIBKTX\_FEATURE\_ETC\_UNPACK

  These options have been duplicated in the libktx project so they can be
  set when it is the top-level project:
  - LIBKTX\_EMBED\_BITCODE
  - LIBKTX\_WERROR

  When libktx is used as a sub-project, they are set from the similarly
  named KTX-Software options.

  Add a workflow to test configuring and building a standalone libktx.

  Along the way, fixed a bug, that caused configurations in the linux.yml
  workflow that were supposed to incorporate OpenCL to not do so. However
  it is not possible to run the tests on those configurations as the
  Actions runners report the cpu type as "generic" so Portable OpenCL
  (POCL) can't figure out what to do.

  Also along the way, update the ignore lists in all the workflow files to
  include newer workflows files.

### Python Bindings

* Bump default version number. (#1161) (6c474d862) (@MarkCallow)

  Various other issues were encountered and fixed while checking the
  version number related changes.

  1. Fix `-a` option of `mkversion`. It did not visit `lib/src` and ended
     with an error due to a misplaced `fi`.
  2. Fix RE in _ktx convert_ that resulted in an extra space in an edited
     KTXwriterScParams when the original `zcmp` option had a parameter.
  3. Fix `ktxTexture2_IsHDR` which was only checking for a FLOAT qualifier
     on DFD samples for the ASTC color model. Add a test for it.

  Fixes #1112.

* Remove `--rec2020` flag and add range mapping functionality to `ktx create` and `ktx extract` (#1157) (20211a217) (@ViNeek)

  This PR fixes #1142 and fixes #1127.

  For #1127, it removes the flag from `ktx create` and `ktx encode` and
  makes sure that the profile is automatically inferred from the loaded
  EXR file.

  For #1142 It adds 3 knew flags to `ktx create`, namely,
  `map-range-auto`, `map-range-offset` and `map-range-scale` to be used
  for mapping of floating point values in HDR formats.
   
  `ktx extract` is also updated to handle inverse mapping of values when
  the `KTXmapRange` key/value pair is present in a KTX2 file.

  Currenlty, `offset` and `scale` are not used for the alpha channel.
  Currently, range mapping only works with floating point data types and
  the new HDR formats.

* Expose HDR support in Python binding (#1150) (5444378e1) (@MarkCallow)

  Fixes #1145.

  Additional fixes:
  - Adds `num_layers` property that was curiously missing.
  - Exposes ktxTexture[12]_IsHDR in the c API.
  - Reformats documentation comments for UASTC HDR ktxBasisParams to
     reduce line width.

* CMake improvements for installing and dependencies (#1133) (8b925f225) (@MarkCallow)

  * Handle installation of `KHR/khr_df.h` for KTX-Software in KTX-Software
    (`lib/CMakeLists.txt`) avoiding some confusion and making installation
    in a framework possible.
  * Use add\_lib\_dependency macro to add dfdutils dependency to libktx and
    remove unnecessary PUBLIC export.
  * Use FILE\_SETs for installation of both dfdutils and libktx include
    files to handle the KHR subdirectory, with a workaround for framework
    installation.
  * Up the cmake minimum version to 3.23 in both dfdutils CMakeLists and
    lib CMakeLists.txt This is so
      - FILE\_SET can be used
      - KTX-Software can override options settings with ordinary variables
        thus hiding unnecessary DFDUTILS options from the CMake GUI.
  * Add option to turn off dfdutils docs build so `lib/CMakeLists.txt` can
    do so.
  * Update dfdutils install to install library and `dfd.h` in addition to
    `KHR/khr_df.h`. This is behind an option so the KTX-Software build can
    turn it off.
  * Include GNUInstallDirs to get proper values for CMAKE\_INSTALL\_<dir>.
  * Normalize the indentation in dfdutils/CMakeLists.txt.
  * Improve command clarity.

  `cmake --build ... --target install` and the installable packages have
  been tested on all 3 desktop platforms and the framework in the iOS .zip
  file and content of the Android .zip file have been verified. All have
  the expected content.

  Fixes #1121. Fixes #1123.

* Improved HDR support (#1124) (35d9f8d53) (@ViNeek)

  Fixes #1122, #1119, #1120 and #1125.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Remove legacy tools (#1110) (05b0e0289) (@MarkCallow)

  - Remove legacy tools, documentation and references to them.
  - Reorganize and normalize the names of still needed test images (now
    under tests/resources).
  - Rewrite genktx2 script to generate needed .ktx2 resources using `ktx`
    suite.
  - Fix missing features and bugs in suite discovered when rewriting
    genktx2:
    - Fix default imageio scanline reader to only buffer and rescale a
      scanline's worth of pixels.
    - Add channel add/remove functionality to default scanline reader
      as `ktx create` always requests 4 channels.
    - Fix ImageT::yflip to not attempt to yflip a 1 row image thus not
      crash.
    - Add listing file support to `ktx create` so multiple input files names
      can be provided as a list in another file
    - Fix so sample qualifierLinear does not cause sameUnitAllChannels not
      to be set.
  - Fix unit tests to compile with c++20 and simplify file handling by
    using std::filesystem.
    - texturetests, transcodetests, streamtests & threadtests now use
      std::format. This PR works around GCC 11's (the GCC version on
      Ubuntu 22.04 CI runners) lack of support for std::format by using
      fmt::format in that case.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

* Create libktx project (#1071) (6765255be) (@MarkCallow)

  Move the libktx build from the root CMakeLists.txt to its own in the
  `lib` directory. Although still within KTX-Software, the new libktx
  project `CMakeLists.txt` can be used as either a sub-project of
  KTX-Software or as a standalone project to build just libktx.

  This has been done due to both direct requests, such as
  https://github.com/KhronosGroup/KTX-Software/discussions/995 and
  indirect requests such as complaints that the root CMakeLists is too
  complicated. The effort has led to some useful cleanup and better
  modularization.

  The following FEATURE options have been moved to the libktx project and
  consequently renamed with a `LIBKTX` prefix:
  - LIBKTX\_FEATURE\_KTX1 
  - LIBKTX\_FEATURE\_KTX2
  - LIBKTX\_FEATURE\_VK\_UPLOAD
  - LIBKTX\_FEATURE\_GL\_UPLOAD
  - LIBKTX\_FEATURE\_ETC\_UNPACK

  These options have been duplicated in the libktx project so they can be
  set when it is the top-level project:
  - LIBKTX\_EMBED\_BITCODE
  - LIBKTX\_WERROR

  When libktx is used as a sub-project, they are set from the similarly
  named KTX-Software options.

  Add a workflow to test configuring and building a standalone libktx.

  Along the way, fixed a bug, that caused configurations in the linux.yml
  workflow that were supposed to incorporate OpenCL to not do so. However
  it is not possible to run the tests on those configurations as the
  Actions runners report the cpu type as "generic" so Portable OpenCL
  (POCL) can't figure out what to do.

  Also along the way, update the ignore lists in all the workflow files to
  include newer workflows files.







### Build Scripts and CMake files

* Fix syntax error. (ba74b8b85) (@MarkCallow)

* Force primary CTS platform for testing. (368132f76) (@MarkCallow)

  This is a hack. If it is decided to keep this feature it needs to be
  done properly, e.g. by passing an argument from the manual workflow
  to the those it is using.

* Add explanatory note. (0bb8a0b65) (@MarkCallow)

* Bump default version number. (#1161) (6c474d862) (@MarkCallow)

  Various other issues were encountered and fixed while checking the
  version number related changes.

  1. Fix `-a` option of `mkversion`. It did not visit `lib/src` and ended
     with an error due to a misplaced `fi`.
  2. Fix RE in _ktx convert_ that resulted in an extra space in an edited
     KTXwriterScParams when the original `zcmp` option had a parameter.
  3. Fix `ktxTexture2_IsHDR` which was only checking for a FLOAT qualifier
     on DFD samples for the ASTC color model. Add a test for it.

  Fixes #1112.

* Fix build issues (#1160) (f2d222d8a) (@MarkCallow)

  1. Suppress what looks to be a bogus stringop-overflow warning from GCC,
     observed with both versions 13 and 15.
  2. Change outdated SSE and OPENCL option names to current names so they
     are acted on properly.

  Given the issues being fixed here, I am unable to understand why our CI
  builds have been green.

* Remove `--rec2020` flag and add range mapping functionality to `ktx create` and `ktx extract` (#1157) (20211a217) (@ViNeek)

  This PR fixes #1142 and fixes #1127.

  For #1127, it removes the flag from `ktx create` and `ktx encode` and
  makes sure that the profile is automatically inferred from the loaded
  EXR file.

  For #1142 It adds 3 knew flags to `ktx create`, namely,
  `map-range-auto`, `map-range-offset` and `map-range-scale` to be used
  for mapping of floating point values in HDR formats.
   
  `ktx extract` is also updated to handle inverse mapping of values when
  the `KTXmapRange` key/value pair is present in a KTX2 file.

  Currenlty, `offset` and `scale` are not used for the alpha channel.
  Currently, range mapping only works with floating point data types and
  the new HDR formats.

* Set link option only for Emscripten. (#1153) (cbdc1a6ac) (@MarkCallow)

* Fix escaping of underbars in variable names (e2e6d4642) (@MarkCallow)

  so it only happens if the name is not in a backquoted section.

* Reinstate ktxtools\_mainpage. (4ffa3f0d0) (@MarkCallow)

  Fix issues in its content.

* Fix validation warning for VK\_IMAGE\_LAYOUT\_SHADER\_READ\_ONLY\_OPTIMAL c… (#1148) (2914abe3a) (@MarkCallow)

  …ase.

  For the `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` case of
  `setImageLayout`, where `dstAccessMask` is `VK_ACCESS_SHADER_READ_BIT`,
  setting`destStageFlags` to `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT` is
  invalid usage and is flagged by the validator in recent Vulkan SDKs.
  Change `dstStageFlags` for this case to
  `VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT`.

  Fixes #1092.

  Includes completely unrelated fixes for warnings that are new in
  Emscripten 5.0.3 so that CI builds are successful.
  - Set -Wno-experimental to disable compile- and link-time warnings that
  sdl3 is still experimental.
  - Change `EMSCRIPTEN` pre-processor macro to `__EMSCRIPTEN__` to stop
  deprecation warnings.

* Remove dead code. (535c77df6) (@MarkCallow)

* CMake improvements for installing and dependencies (#1133) (8b925f225) (@MarkCallow)

  * Handle installation of `KHR/khr_df.h` for KTX-Software in KTX-Software
    (`lib/CMakeLists.txt`) avoiding some confusion and making installation
    in a framework possible.
  * Use add\_lib\_dependency macro to add dfdutils dependency to libktx and
    remove unnecessary PUBLIC export.
  * Use FILE\_SETs for installation of both dfdutils and libktx include
    files to handle the KHR subdirectory, with a workaround for framework
    installation.
  * Up the cmake minimum version to 3.23 in both dfdutils CMakeLists and
    lib CMakeLists.txt This is so
      - FILE\_SET can be used
      - KTX-Software can override options settings with ordinary variables
        thus hiding unnecessary DFDUTILS options from the CMake GUI.
  * Add option to turn off dfdutils docs build so `lib/CMakeLists.txt` can
    do so.
  * Update dfdutils install to install library and `dfd.h` in addition to
    `KHR/khr_df.h`. This is behind an option so the KTX-Software build can
    turn it off.
  * Include GNUInstallDirs to get proper values for CMAKE\_INSTALL\_<dir>.
  * Normalize the indentation in dfdutils/CMakeLists.txt.
  * Improve command clarity.

  `cmake --build ... --target install` and the installable packages have
  been tested on all 3 desktop platforms and the framework in the iOS .zip
  file and content of the Android .zip file have been verified. All have
  the expected content.

  Fixes #1121. Fixes #1123.

* Add HDR support to vkloadtests (#1130) (dcc434604) (@MarkCallow)

  Invoke with `--hdr` to get an R16G16B16 HDR rendering surface.

  * Add support for transcoding the new HDR formats.
  * Add a sample in each of UASTC HDR 4x4 and UASTC HDR 6x6i.

* HDR Support for libktx and tools (#1100) (c0a32ef23) (@ViNeek)

  The pull request adds initial support for HDR data in libktx and tools

  In particular,

  1. The CLI ktx tool is updated so that:

  - the `ktx info` command supports **UASTC HDR** payload formats as
  defined in the KTX Specification.
  - the `ktx validate` command implements new validation clauses related
  to the **UASTC HDR** payload formats
  - the `ktx encode` command now accepts **uastc-hdr-4x4** and
  **uastc-hdr-6x6i** as codec strings.
  - These codecs require the input file format to be either
  **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**.
  - the `ktx create` command when the **encode** parameter is used,
  accepts **uastc-hdr-4x4** and **uastc-hdr-6x6i** for the **codec**
  parameter
  - if the input is raw, these codecs require the format parameter to be
  either **R16G16B16\_SFLOAT** or **R16G16B16A16\_SFLOAT**;
  - if the input is not raw, the input files must be EXR with half-float
  data type
  - the ktx transcode supports the two new codecs for input files: 
  - **uastc-hdr-4x4** inputs must accept three targets: **rgba16f**,
  **astc-hdr-4x4**, and **bc6hu**
  - **uastc-hdr-6x6i** inputs must accept three targets: **rgba16f**,
  **astc-hdr-6x6**, and **bc6hu**
  - if an input file is neither of these two formats, the targets are
  rejected,
  - the `ktx extract` command supports the two new codecs for input files 
  - **uastc-hdr-4x4** inputs may be extracted as raw, or transcoded as in
  `ktx transcode`
  - **uastc-hdr-6x6i** inputs require transcoding as in `ktx transcode`
  when used with this command
  - the `ktx compare` command, when used with **UASTC HDR 4x4** or **UASTC
  HDR 6x6** data and pixel comparison is requested, the image is
  transcoded to uncompressed half-float values prior to comparison

  2. The existing JavaScript bindings is updated to support the new
  functionality, specifically transcoding of UASTC HDR payloads

  Incidentally fixes #1109.

* Fix installation of test ktx files to resources on Windows. (#1117) (d58b713c1) (@MarkCallow)

  ensure\_runtime\_dependencies\_win was still trying copy a whole directory
  which is no longer true after the reorganization.

* Remove legacy tools (#1110) (05b0e0289) (@MarkCallow)

  - Remove legacy tools, documentation and references to them.
  - Reorganize and normalize the names of still needed test images (now
    under tests/resources).
  - Rewrite genktx2 script to generate needed .ktx2 resources using `ktx`
    suite.
  - Fix missing features and bugs in suite discovered when rewriting
    genktx2:
    - Fix default imageio scanline reader to only buffer and rescale a
      scanline's worth of pixels.
    - Add channel add/remove functionality to default scanline reader
      as `ktx create` always requests 4 channels.
    - Fix ImageT::yflip to not attempt to yflip a 1 row image thus not
      crash.
    - Add listing file support to `ktx create` so multiple input files names
      can be provided as a list in another file
    - Fix so sample qualifierLinear does not cause sameUnitAllChannels not
      to be set.
  - Fix unit tests to compile with c++20 and simplify file handling by
    using std::filesystem.
    - texturetests, transcodetests, streamtests & threadtests now use
      std::format. This PR works around GCC 11's (the GCC version on
      Ubuntu 22.04 CI runners) lack of support for std::format by using
      fmt::format in that case.

* Add convert command (#1099) (0eaebb015) (@MarkCallow)

  For converting ktx v1 files.

  Fixes formatting error in encode man page (doxygen).

* Add lock around transcoder initialization (#1098) (964acdc17) (@MarkCallow)

  Fixes #1087.

  Add threadtests - though unsure how useful they are.

  Update all libktx tests for c++20 as new tests need it for std::barrier.

  Fixes basisu\_c\_binding build when ktx\_read target is not included in
  build.

  Big thanks to @vmwalker for reporting the problem and providing the fix.

* Combine lib dependencies into static libktx on all desktop platforms (#1090) (5a07bc6f8) (@MarkCallow)

  Previously this was only done on macOS - using `libtool`. Changed to
  a simple, though hard to find, cross-platform way to do this via CMake.

  Add CI test of build and use of a static library.

  Remove macOS 13 build from CI as these runner images have been
  retired from GitHub Actions.

* Fix use of libktx project as subproject outside of KTX-Software (#1089) (7f0f889e3) (@MarkCallow)

  #### The Fix

  * Always include `cmake/codesign.cmake` and `cmake/cputypetest.cmake`.
    Add `include_guard()` to them to prevent multiple inclusion.
  * Add host project for testing sub-project use and update workflow
     to build and run it and libktx.

  #### Other Changes
  * Add options to select building of full or read-only libktx.
  * Fix macOS build when CODE\_SIGN\_IDENTITY not set. Fixes
    both libktx and the larger KTX-Software project.

  Fixes #1083

* mkversion fix: make RE treat $(pwd) value literally. (#1088) (998a719d2) (@MarkCallow)

  Fixes #1084.

* Revise mkversion invocation (#1085) (2aa509df4) (@dg0yt)

  This is an *untested* rewrite of the line invoking `mkversion` on
  Windows, based on past vcpkg experience. (The current port version 4.4.2
  doesn't invoke mkversion.)

  The original `"\"...\" ...."` fails *for some configurations*.
  It is a single CMake list item, with embedded quotes which go into a
  build system rule which may or may not get the inner quotes correctly
  for the actual invocation of the command.

  The rule of thumb is:
  - Quote for CMake.  
    There is *only outer quotes* for items in a *cmake list*.
  - Do not assume a particular shell for commands executed by the build
  system.
    There are different generators for different build systems.
  With ninja build files, commands may be executed via a shell or
  directly, depending on the system.
    With any type of *inner quotes*, portability will be limited.

  The new line is a CMake list of command and items, with quoting for
  CMake when items can expand with special characters inside.
  (Now this newest version contains an embedded CMake variable
  `${MKV_VERSION_OPT}` which I didn't quote because I assume it is of list
  type, i.e. potentially expanding to multiple command line arguments.)

* Fix creating static ktx\_read convenience lib (#1081) (4b46cda2a) (@dg0yt)

  Fixes "duplicate member name" warnings:
  ~~~
  [130/148] : [...] libtool -static -o
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx\_read.a
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx.a
  /Users/vcpkg/Data/b/ktx/x64-osx-rel/external/astc-encoder/Source/libastcenc-avx2-static.a

  /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/libtool:
  warning for architecture: x86\_64h duplicate member name
  'astcenc\_averages\_and\_directions.cpp.o' from '/Users/vcpkg/Data/b/ktx/x64-osx-rel/external/astc-encoder/Source/libastcenc-avx2-static.a(astcenc\_averages\_and\_directions.cpp.o)'
  and
  '/Users/vcpkg/Data/b/ktx/x64-osx-rel/libktx.a(astcenc\_averages\_and\_directions.cpp.o)'
  ~~~

* Update basis\_universal (#1080) (422f501ea) (@MarkCallow)

  master, in the fork from which we pull basis\_universal, has been updated
  to basis\_universal master as of 2025.11.22, which includes all our
  previous warning fixes. The changes there have been merged to the
  `cmake_fixes` branch we pull from.

  Also included:
  * fix: change basis-related cmake option names to match those in
  basis\_universal and update workflows to set the new names;
  * fix: deploy x86\_64 package, broken when build moved to x86\_64 runner;
  * enhancement: apply changes resulting from a review of the cmake\_fixes
  PR for basis\_universal.

  git subrepo clone --branch=cmake\_fixes --force https://github.com/KhronosGroup/basis\_universal.git external/basis\_universal

  subrepo:
    subdir:   "external/basis\_universal"
    merged:   "daf79c6ee"
  upstream:
    origin:   "https://github.com/KhronosGroup/basis\_universal.git"
    branch:   "cmake\_fixes"
    commit:   "daf79c6ee"
  git-subrepo:
    version:  "0.4.9"
    origin:   "https://github.com/MarkCallow/git-subrepo.git"
    commit:   "4f60dd7"

  Fixes #1079.

* Required changes to work with basis\_universal release 1.60+. (ac2edf196) (@MarkCallow)

  * Stop direct inclusion of sources. Use their cmake project to build the
    library as a subproject.
  * Adapt to renamed functions.
  * Update reference files for changed encoder results.

  Unrelated but necessary fixes:

  * fix bug in build\_macos\_sh that preventing running of tests when
    cross-compiling for x86\_64 on arm64;
  * fix CI to build x86\_64 macOS package on an x86\_64 runner as the
    x86\_64 Java and Python tests will not run via Rosetta

* Add KTX\_FEATURE\_JS option controlling whether `ktx_js[_read]` are built. (#1073) (952d74f1d) (@kring)

  (Reopening #1072, now targeting `main`)

  Adds a new CMake option called `KTX_FEATURE_JS`. It defaults to ON, but
  when set to OFF, `ktx_js` and `ktx_js_read` are not built, even under
  Emscripten.

  My use-case for this is compiling [Cesium for
  Unity](https://github.com/CesiumGS/cesium-unity) for the web. Compiling
  Unity applications to WebAssembly requires using Unity's version of
  Emscripten, which is quite old (v3.1.39), and is not able to compile
  `ktx_js` successfully. This isn't really worth fixing because the
  JavaScript bindings aren't used in this context anyway. KTX
  functionality is only called from Emscripten'd native code, never from
  regular JS code.

  So with this low-impact PR, it's easy to disable this unnecessary part
  of the KTX software suite in order to get a successful build.

  The slightly bigger picture here is to then expose this via a vcpkg
  feature flag.

* Create libktx project (#1071) (6765255be) (@MarkCallow)

  Move the libktx build from the root CMakeLists.txt to its own in the
  `lib` directory. Although still within KTX-Software, the new libktx
  project `CMakeLists.txt` can be used as either a sub-project of
  KTX-Software or as a standalone project to build just libktx.

  This has been done due to both direct requests, such as
  https://github.com/KhronosGroup/KTX-Software/discussions/995 and
  indirect requests such as complaints that the root CMakeLists is too
  complicated. The effort has led to some useful cleanup and better
  modularization.

  The following FEATURE options have been moved to the libktx project and
  consequently renamed with a `LIBKTX` prefix:
  - LIBKTX\_FEATURE\_KTX1 
  - LIBKTX\_FEATURE\_KTX2
  - LIBKTX\_FEATURE\_VK\_UPLOAD
  - LIBKTX\_FEATURE\_GL\_UPLOAD
  - LIBKTX\_FEATURE\_ETC\_UNPACK

  These options have been duplicated in the libktx project so they can be
  set when it is the top-level project:
  - LIBKTX\_EMBED\_BITCODE
  - LIBKTX\_WERROR

  When libktx is used as a sub-project, they are set from the similarly
  named KTX-Software options.

  Add a workflow to test configuring and building a standalone libktx.

  Along the way, fixed a bug, that caused configurations in the linux.yml
  workflow that were supposed to incorporate OpenCL to not do so. However
  it is not possible to run the tests on those configurations as the
  Actions runners report the cpu type as "generic" so Portable OpenCL
  (POCL) can't figure out what to do.

  Also along the way, update the ignore lists in all the workflow files to
  include newer workflows files.

* ktx create: add premultiply alpha option (#1049) (83d84dce6) (@jiangzhhhh)

  * Add alpha pre-multiply function to image.hpp.

  * Update base Texture load test to check for pre-multiplied textures
    and add a sample to the tests.

* Determine tools version at build time not config time (#1068) (0b3fad6a6) (@MarkCallow)

  Otherwise the tools version may not reflect recent changes in
  the worktree.

  Add comments pointing to docs for git describe output.
