<!-- Copyright 2025, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.4.2
### Notice
v4.4.2 is an emergency release that replaces v4.4.1 which has been withdrawn. It
fixes the version number in the release assets. There are no other changes
compared to v4.4.1.

### Summary

* `ktxTexture2_DecodeAstc` now exposed in _libktx\_read_ on all platforms and in
  the JS bindings.
* `ktx info` can now show info about KTX v1 files. It and the underlying _libktx_
  function now display GL type and format token names instead of hex values.
* Many bugs and robustness issues have been fixed. Many of these address changes
  in CI runner images and the latest compilers.
* ASTC encoder updated to 5.3.0.
* LodePNG updated to 20250506.
* Building with Visual Studio 2019 is no longer supported.

__The legacy tools will be removed in Release 4.5. Adjust your workflows accordingly.__

### New Features in v4.4.2
#### libktx functions

* `ktxTexture2_DecodeAstc`, which decodes an ASTC format texture to an
  uncompressed format, is now available in _libktx\_read_ on all platforms and in
  both the _libktx_ and _libktx\_read_ JS bindings.
* `ktxPrintKTX1InfoTextForStream`, which prints information about a KTX v1 file
  and was previously internal, is now exposed.

### Notable Fixes in v4.4.2

* A bug in mipmap generation in `ktx create` that led to sRGB images being
  resampled without first decoding to linear has been fixed. If you have affected
  KTX files you should regenerate your textures from their source images.
* _libktx\_read_ no longer includes the ASTC encode functions, only the decode
  functions.
* A bug that caused a hang in `ktx create`, when the source image is an RGB PNG
  file with an sBIT chunk and an alpha component is being added to the texture
  being created, has been fixed.


### Known Issues

* Files deflated with zlib using *libktx* compiled with GCC and run on x86\_64 may not be bit-identical with those using *libktx* compiled with GCC and run on arm64.

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using the `ktx create` tool, or use the `--resize` feature to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of `ktx validate`.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.

### Changes since v4.4.0 (by part)
### libktx

* 4.4.1 release prep (#1063) (0b10eb17e) (@MarkCallow)

* Add v1 support to ktx info (#1060) (3cd9e3447) (@MarkCallow)

* Remove mentions of retired edgewise-consulting.com. (#1058) (0306d6a61) (@MarkCallow)

* Export ktxTexture2\_DecodeAstc in JS bindings (#1034) (64a69009b) (@MarkCallow)

* Restore rotted bits: update vcpkg caching and pyktx to latest Python (#1033) (f753fcabe) (@MarkCallow)

* Fix for Emscripten 4.0.9 (#1026) (1a983763c) (@MarkCallow)

* Fix memory leaks. (#1007) (504b96247) (@MarkCallow)

### Tools

* Add v1 support to ktx info (#1060) (3cd9e3447) (@MarkCallow)

* Remove mentions of retired edgewise-consulting.com. (#1058) (0306d6a61) (@MarkCallow)

* Document that --generated-mipmap can't be used with --raw. (#1057) (1d7d44465) (@MarkCallow)

* Update fmt to latest release (v11.2) (#1056) (f5654c2b0) (@MarkCallow)

* Fix hang when adding alpha and PNG input has sBIT chunk. (#1054) (d9a0c2bd5) (@MarkCallow)

* Set color space in input image prior to resample. (#1051) (1daca0cdc) (@MarkCallow)

* Use relative rpaths to find installed library on macOS (#1046) (d47320c24) (@MarkCallow)

* Shut up warnings clang with libstdc++ emits about non-virtual destructors (#1012) (fbb5412f5) (@DanielGibson)

* Fix CLI error handling for --normalize. (#1016) (17d206239) (@MarkCallow)

* Update LodePNG to version 20241228, (#1015) (6d1fc82ca) (@MarkCallow)

### JS Bindings

* Export ktxTexture2\_DecodeAstc in JS bindings (#1034) (64a69009b) (@MarkCallow)

* Fix for Emscripten 4.0.9 (#1026) (1a983763c) (@MarkCallow)

### Java Bindings

* Use relative rpaths to find installed library on macOS (#1046) (d47320c24) (@MarkCallow)

### Python Bindings

* Restore rotted bits: update vcpkg caching and pyktx to latest Python (#1033) (f753fcabe) (@MarkCallow)

* Add option to use virtual environment for Python. (#1029) (b167e968c) (@MarkCallow)

* Bump setuptools from 70.0.0 to 78.1.1 in /interface/python\_binding (#1025) (94d0c3a81) (@dependabot[bot])

* Fix python deprecation warning. (#1018) (dac48df00) (@MarkCallow)

### External Package Dependencies

* Remove mentions of retired edgewise-consulting.com. (#1058) (0306d6a61) (@MarkCallow)

* Update fmt to latest release (v11.2) (#1056) (f5654c2b0) (@MarkCallow)

* Migrate loadtest apps to SDL3 (#1055) (443e12238) (@MarkCallow)

* Update ASTC encoder to 5.3.0 (#1036) (f6f0b9a1d) (@MarkCallow)

* Update lodepng to version 20250506 (#1035) (f9c73388a) (@MarkCallow)

* GCC14/C++23 compatibility fix (#1014) (f3f6b3b69) (@alexge50)

* Update LodePNG to version 20241228, (#1015) (6d1fc82ca) (@MarkCallow)

### Tests

* Add v1 support to ktx info (#1060) (3cd9e3447) (@MarkCallow)

* Remove mentions of retired edgewise-consulting.com. (#1058) (0306d6a61) (@MarkCallow)

* Update CTS ref to merged tests. (65b0031d7) (@MarkCallow)

* Document that --generated-mipmap can't be used with --raw. (#1057) (1d7d44465) (@MarkCallow)

* Update fmt to latest release (v11.2) (#1056) (f5654c2b0) (@MarkCallow)

* Migrate loadtest apps to SDL3 (#1055) (443e12238) (@MarkCallow)

* Fix hang when adding alpha and PNG input has sBIT chunk. (#1054) (d9a0c2bd5) (@MarkCallow)

* Set color space in input image prior to resample. (#1051) (1daca0cdc) (@MarkCallow)

* Use relative rpaths to find installed library on macOS (#1046) (d47320c24) (@MarkCallow)

* Clarify platforms where unit tests not supported. (#1040) (f9f36940b) (@MarkCallow)

* Minor build fixes (#1039) (8dfb89507) (@MarkCallow)

* Update for Vulkan SDK 1.4.313. (#1037) (d72218c6a) (@MarkCallow)

* Export ktxTexture2\_DecodeAstc in JS bindings (#1034) (64a69009b) (@MarkCallow)

* Restore rotted bits: update vcpkg caching and pyktx to latest Python (#1033) (f753fcabe) (@MarkCallow)

* Fix handling of multiple files with spaces in names. (#1030) (b2f4da2aa) (@MarkCallow)

* Update CTS ref for merged test updates. (b9218bc50) (@MarkCallow)

* Fix CLI error handling for --normalize. (#1016) (17d206239) (@MarkCallow)

* Linux and MacOS workflows (#1004) (520dc8f89) (@MathiasMagnus)



### Build Scripts and CMake files

* Add force-fetch-provoking-tag-annotation workaround (e5c085b51) (@MarkCallow)

* Add options. (16a24e087) (@MarkCallow)

* Add v1 support to ktx info (#1060) (3cd9e3447) (@MarkCallow)

* Migrate loadtest apps to SDL3 (#1055) (443e12238) (@MarkCallow)

* Fix hang when adding alpha and PNG input has sBIT chunk. (#1054) (d9a0c2bd5) (@MarkCallow)

* Use relative rpaths to find installed library on macOS (#1046) (d47320c24) (@MarkCallow)

* Clarify platforms where unit tests not supported. (#1040) (f9f36940b) (@MarkCallow)

* Minor build fixes (#1039) (8dfb89507) (@MarkCallow)

* Update for Vulkan SDK 1.4.313. (#1037) (d72218c6a) (@MarkCallow)

* Export ktxTexture2\_DecodeAstc in JS bindings (#1034) (64a69009b) (@MarkCallow)

* Restore rotted bits: update vcpkg caching and pyktx to latest Python (#1033) (f753fcabe) (@MarkCallow)

* Add option to use virtual environment for Python. (#1029) (b167e968c) (@MarkCallow)

* Fix: Install graphviz if FEATURE\_DOCS ON (#1028) (e69101917) (@MarkCallow)

* Fix for Emscripten 4.0.9 (#1026) (1a983763c) (@MarkCallow)

* Enable use of Ninja Multi-Config generator for Linux builds (#1017) (161878025) (@MarkCallow)

* Update LodePNG to version 20241228, (#1015) (6d1fc82ca) (@MarkCallow)

* Fix issues in CMakeLists.txt (see #996) (#998) (c033ac8fa) (@DanielGibson)

* Linux and MacOS workflows (#1004) (520dc8f89) (@MathiasMagnus)
