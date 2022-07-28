<!-- Copyright 2022, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.1.0
### New Features in v4.1.0

* ARM's ASTC encoder has been added to `libktx`. As a result you can now use `toktx` to create KTX files with ASTC encoded payloads. Thanks to @wasimabbas-arm.

* Full normal map handling has been added. 3-component normal maps can be
converted to 2-component and the components separated into the RGB and alpha channels of ASTC, ETC1S or UASTC compressed textures. A `--normalize` option has been added to `toktx` to convert an input normal map to unit normals which are needed to allow the third component to be recreated in a shader.
Thanks to @wasimabbas-arm.

* A Java wrapper and JNI module for `libktx` has been added. Thanks to @ShukantPal.

* An install package for Apple Silicon has been added.

* An install package for Windows Arm-64 has been added. Thanks to @Honeybunch.

* The formerly internal `ktxStream` class has been exposed enabling possibilities such as wrapping a ktxStream around a C++ stream so that textures can be created from the C++ stream's content. See [sbufstream.h](https://github.com/KhronosGroup/KTX-Software/blob/master/utils/sbufstream.h). Thanks to @UberLambda.

* `ktx2check` now verifies BasisLZ supercompression data by performing a transcode.

### Significant Changes since v4.0.0

* Basis Universal has been updated to version 1.16.3.
    * The ETC1S encoder performance is now approximately 30% faster.
    * Optional OpenCL support has been added to the ETC1S encoder. Add `-D SUPPORT_OPENCL` when configuring the CMake build to enable it. As OpenCL may not be any faster when encoding individual files - it highly depends on your hardware - it is disabled in the default build and release packages.

* Windows install packages are now signed.

* Textures with Depth-stencil formats are now created with DFDs and alignments matching the KTX v2 specification.

* Specifying `--layers 1` to `toktx` now creates an array texture with 1 layer. Previously it created a non-array texture.

* `--normal_map` in `ktxsc` and `toktx` has been replaced by `--normal_mode` which converts 3-component maps to 2-component as well as optimizing the encoding. To prevent the conversion, also specify `--input_swizzle rgb1`.

### Known Issues in v4.1.0.

* `toktx` will not read JPEG files with a width or height > 32768 pixels.

* `toktx` will not read 4-component JPEG files such as those sometimes created by Adobe software where the 4th component can be used to re-create a CMYK image.

* Users making Basisu encoded or block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize images appropriately using the --resize feature of `toktx`.  In general the dimensions of block compressed textures must be a multiple of the block size and for WebGL 1.0 must be a power of 2. For portability glTF's _KHR\_texture\_basisu_ extension requires texture dimensions to be a multiple of 4, the block size of the Universal texture formats.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor the GL loader support depth/stencil textures.

### Changes since v4.0.0 (by part)
### libktx

* git subrepo push lib/dfdutils (dd799a9b) (@null)

* Remove incorrect use of ktxTexture2\_WriteTo... (7d91d62e) (@MarkCallow)

* Regularize Tools (#594) (870b9fff) (@MarkCallow)

* Fixing build for arm64 Windows (#582) (b995ac33) (@Honeybunch)

* Update astc-encoder (#592) (a6bcd33d) (@MarkCallow)

* Fix missing documentation and compile warning. (#591) (ed9e7253) (@MarkCallow)

* Update astc encoder (#586) (1cb97511) (@MarkCallow)

* Release memory before early exit. (#584) (a4fddf6b) (@kacprzak)

* Introduce proper vulkan initialization (#570) (bb9babcb) (@rHermes)

* Using cmake's MINGW variable to detect proper ABI (#579) (a70e831e) (@Honeybunch)

* Fix handling of combined depth-stencil textures (#575) (e4bf1aaa) (@MarkCallow)

* Fix build on Mingw (#574) (1f07cb07) (@Honeybunch)

* Prepare Release 4.1. (#571) (4a52fe45) (@MarkCallow)

* git subrepo pull (merge) lib/astc-encoder (51f47631) (@MarkCallow)

* git subrepo pull (merge) lib/dfdutils (7c24a986) (@MarkCallow)

* git subrepo pull (merge) lib/dfdutils (c5abc161) (@MarkCallow)

* Farewell GYP. :-( (f1f04a7e) (@MarkCallow)

* Miscellaneous fixes (#558) (66f6d750) (@MarkCallow)

* Fix new in clang 13.1 (Xcode13.3) warnings (#553) (b8d462b0) (@MarkCallow)

* Fix non-clang warnings (#549) (4e7e40a0) (@MarkCallow)

* Split each build configuration into a separate CI job.  (#546) (9d1204cc) (@MarkCallow)

* Update to Basis1.16.3 (#543) (c65cfd0d) (@MarkCallow)

* Remove image.hpp dependency (#542) (9fde96b9) (@wasimabbas-arm)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@MarkCallow)

* git subrepo pull (merge) lib/astc-encoder (#540) (d98aa680) (@wasimabbas-arm)

* git subrepo pull (merge) lib/astc-encoder (#537) (dbfeb82a) (@wasimabbas-arm)

* Add astc perceptual mode support (#534) (57e62de1) (@wasimabbas-arm)

* Improve Astc & BasisU normal map support (#493) (2d6ff949) (@wasimabbas-arm)

* git subrepo pull lib/dfdutils (5ff4811c) (@MarkCallow)

* git subrepo push lib/dfdutils (ce2a4619) (@MarkCallow)

* Calculate dst buffer size with ZSTD\_compressBound. (#527) (81d2be5c) (@MarkCallow)

* Remove extraneous token concatenation operator. (a8f4a71d) (@MarkCallow)

* Fix malloc/delete pair. (0a3fe5b1) (@sergeyext)

* Manually update git-subrepo parent (929c75c3) (@wasimabbas-arm)

* git subrepo pull (merge) lib/astc-encoder (f5daffea) (@wasimabbas-arm)

* Fix parent commit pointer. (1a356d0e) (@MarkCallow)

* git subrepo pull (merge) lib/basisu (24c9f7bb) (@MarkCallow)

* Move common params out from ETC1S case. (a2ccc90e) (@MarkCallow)

* Remove transferFunction from astc options (#482) (1f085d30) (@wasimabbas-arm)

* Fix leak in zstd inflation. Fixes #465. (720b6cf3) (@MarkCallow)

* Support array and 3d textures. (#468) (b0532530) (@MarkCallow)

* Add more astc tests (#460) (14284e7d) (@wasimabbas-arm)

* Add astc support (#433) (da435dee) (@wasimabbas-arm)

* Actually byte swap keyAndValueByteSize values. Fix issue #447. (00118086) (@MarkCallow)

* Add KTXmetalPixelFormat to valid list used by ktxTexture2\_WriteToStream. (871f111d) (@MarkCallow)

* Fix astc-encoder/.gitrepo parent after latest pull. (f99221eb) (@MarkCallow)

* git subrepo pull (merge) lib/astc-encoder (66692454) (@MarkCallow)

* Fix astc-encoder/.gitrepo parent pointer. (f39b13b1) (@MarkCallow)

* Fix memory leak in VkUpload (#448) (2b2b48fa) (@bin)

* Fix: if ("GL\_EXT\_texture\_sRGB") is supported,then srgb should be supported (#446) (13f17410) (@dusthand)

* git subrepo commit (merge) lib/astc-encoder (1264f867) (@MarkCallow)

* git subrepo pull (merge) lib/astc-encoder (15369663) (@MarkCallow)

* Make `ktxStream` public (#438) (78929f80) (@UberLambda)

* git subrepo pull (merge) lib/astc-encoder (535c883b) (@MarkCallow)

* Fix mismatched malloc and delete (#440) (9d42b86f) (@cperthuisoc)

* Cleanup Vulkan SDK environment variables. (354f640e) (@MarkCallow)

* git subrepo pull (merge) lib/astc-encoder (3e75b6a3) (@MarkCallow)

* Remove unneeded parts of astc-encoder. (360d10bb) (@MarkCallow)

* git subrepo clone https://github.com/ARM-software/astc-encoder.git lib/astc-encoder (db359593) (@MarkCallow)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@MarkCallow)

* Minor build tweaks (#407) (6a38a069) (@MarkCallow)

### Tools

* Allow creation of single layer array textures. (#602) (de93656b) (@MarkCallow)

* Close file after successful load (#597) (32d26662) (@AndrewChan2022)

* Regularize Tools (#594) (870b9fff) (@MarkCallow)

* Fix cross-device rename failure (#593) (f020c1ba) (@MarkCallow)

* Fix wrong alignment used when checking VK\_FORMAT\_UNDEFINED files (#585) (c7e4edc7) (@MarkCallow)

* Sign Windows executables, dlls and NSIS installers. (#583) (dc231b32) (@MarkCallow)

* Fix broken bytesPlane0 test. Add extra analysis. (#578) (243ba439) (@MarkCallow)

* Fix handling of combined depth-stencil textures (#575) (e4bf1aaa) (@MarkCallow)

* Farewell GYP. :-( (f1f04a7e) (@MarkCallow)

* Miscellaneous fixes (#558) (66f6d750) (@MarkCallow)

* Add JNI component and integrate Java build & test with CMake (#556) (e29e0996) (@MarkCallow)

* Fix non-clang warnings (#549) (4e7e40a0) (@MarkCallow)

* Fix VS warnings (#544) (8c6b3571) (@wasimabbas-arm)

* Remove image.hpp dependency (#542) (9fde96b9) (@wasimabbas-arm)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@MarkCallow)

* Improve Astc & BasisU normal map support (#493) (2d6ff949) (@wasimabbas-arm)

* Validate BasisU Transcode (#532) (39e2d96e) (@MarkCallow)

* Fix mismatched errors for required and optional index entries. (b8786496) (@MarkCallow)

* fix missing -w flag for ktx2check (eade072d) (@sidsethupathi)

* Remove transferFunction from astc options (#482) (1f085d30) (@wasimabbas-arm)

* Ensure NUL on end of 3d orientation. (74501ef3) (@MarkCallow)

* Support array and 3d textures. (#468) (b0532530) (@MarkCallow)

* Fix checks for mismatched image attributes. (#466) (4eca0ef3) (@MarkCallow)

* Add more astc tests (#460) (14284e7d) (@wasimabbas-arm)

* Add astc support (#433) (da435dee) (@wasimabbas-arm)

* macOS Apple Silicon support (#415) (ebab2ea8) (@atteneder)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@MarkCallow)

* Fix validation errors (#417) (78cd2b01) (@MarkCallow)



### JS Wrappers

* Farewell GYP. :-( (f1f04a7e) (@MarkCallow)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@MarkCallow)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@MarkCallow)

### Java Wrapper

* Sign Windows executables, dlls and NSIS installers. (#583) (dc231b32) (@MarkCallow)

* Workaround FindJNI searching for framework when JAVA\_HOME not set. (#566) (957a198b) (@MarkCallow)

* Miscellaneous fixes (#558) (66f6d750) (@MarkCallow)

* Add JNI component and integrate Java build & test with CMake (#556) (e29e0996) (@MarkCallow)

* Fix warnings in JNI library and update to latest libktx API. (#548) (6f98b3c4) (@ShukantPal)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@MarkCallow)

* Feature: Java bindings for libktx (#481) (a7159924) (@ShukantPal)


