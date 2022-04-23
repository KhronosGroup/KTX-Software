<!-- Copyright 2022, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.1.0
### New Features in v4.1.0

ARM's ASTC encoder has been added to `libktx`.

Full normal map handling has been added. 3-component normal maps can be
converted to 2-component and the components separated into the RGB and alpha channels of ASTC, ETC1S or UASTC compressed textures. A `--normalize` option has been added to `toktx` to convert an input normal map to the unit normals needed.

A Java wrapper and JNI module for `libktx` has been added.

### Significant Changes since v4.0.0

* Basis Universal has been updated to version 1.16.3. Encoder performance is
hugely improved with the ETC1S encoder being 4 times faster.


### Known Issues in v4.1.0.

* `toktx` will not read JPEG files with a width or height > 32768 pixels.

* `toktx` will not read 4-component JPEG files such as those sometimes
created by Adobe software where the 4th component can be used to re-create
a CMYK image.

* Users making Basisu encoded or block compressed textures for WebGL
must be aware of WebGL restrictions with regard to texture size and
may need to resize images appropriately using the --resize feature
of `toktx`.  In general the dimensions of block compressed textures
must be a multiple of the block size and, if
`WEBGL_compressed_texture_s3tc` on WebGL 1.0 is expected to be one
of the targets, then the dimensions must be a power of 2. For
portability glTF's KHR\_texture\_basisu extension requires texture
dimensions to be a multiple of 4, the block size of the Universal texture
formats.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are
non-deterministic across platforms. Results are valid but level
sizes and data will differ slightly.
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60)
in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the
former or `--uastc_rdo_m` for the latter. As with the preceeding issue
results are valid but level sizes will differ slightly. See
[issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

### Changes since v4.0.0 (by part)
### libktx

* Farewell GYP. :-( (f1f04a7e) (@null)

* Miscellaneous fixes (#558) (66f6d750) (@null)

* Fix new in clang 13.1 (Xcode13.3) warnings (#553) (b8d462b0) (@null)

* Fix non-clang warnings (#549) (4e7e40a0) (@null)

* Split each build configuration into a separate CI job.  (#546) (9d1204cc) (@null)

* Update to Basis1.16.3 (#543) (c65cfd0d) (@null)

* Remove image.hpp dependency (#542) (9fde96b9) (@null)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@null)

* git subrepo pull (merge) lib/astc-encoder (#540) (d98aa680) (@null)

* git subrepo pull (merge) lib/astc-encoder (#537) (dbfeb82a) (@null)

* Add astc perceptual mode support (#534) (57e62de1) (@null)

* Improve Astc & BasisU normal map support (#493) (2d6ff949) (@null)

* git subrepo pull lib/dfdutils (5ff4811c) (@null)

* git subrepo push lib/dfdutils (ce2a4619) (@null)

* Calculate dst buffer size with ZSTD\_compressBound. (#527) (81d2be5c) (@null)

* Remove extraneous token concatenation operator. (a8f4a71d) (@null)

* Fix malloc/delete pair. (0a3fe5b1) (@null)

* Manually update git-subrepo parent (929c75c3) (@null)

* git subrepo pull (merge) lib/astc-encoder (f5daffea) (@null)

* Fix parent commit pointer. (1a356d0e) (@null)

* git subrepo pull (merge) lib/basisu (24c9f7bb) (@null)

* Move common params out from ETC1S case. (a2ccc90e) (@null)

* Remove transferFunction from astc options (#482) (1f085d30) (@null)

* Fix leak in zstd inflation. Fixes #465. (720b6cf3) (@null)

* Support array and 3d textures. (#468) (b0532530) (@null)

* Add more astc tests (#460) (14284e7d) (@null)

* Add astc support (#433) (da435dee) (@null)

* Actually byte swap keyAndValueByteSize values. Fix issue #447. (00118086) (@null)

* Add KTXmetalPixelFormat to valid list used by ktxTexture2\_WriteToStream. (871f111d) (@null)

* Fix astc-encoder/.gitrepo parent after latest pull. (f99221eb) (@null)

* git subrepo pull (merge) lib/astc-encoder (66692454) (@null)

* Fix astc-encoder/.gitrepo parent pointer. (f39b13b1) (@null)

* Fix memory leak in VkUpload (#448) (2b2b48fa) (@null)

* Fix: if ("GL\_EXT\_texture\_sRGB") is supported,then srgb should be supported (#446) (13f17410) (@null)

* git subrepo commit (merge) lib/astc-encoder (1264f867) (@null)

* git subrepo pull (merge) lib/astc-encoder (15369663) (@null)

* Make `ktxStream` public (#438) (78929f80) (@null)

* git subrepo pull (merge) lib/astc-encoder (535c883b) (@null)

* Fix mismatched malloc and delete (#440) (9d42b86f) (@null)

* Cleanup Vulkan SDK environment variables. (354f640e) (@null)

* git subrepo pull (merge) lib/astc-encoder (3e75b6a3) (@null)

* Remove unneeded parts of astc-encoder. (360d10bb) (@null)

* git subrepo clone https://github.com/ARM-software/astc-encoder.git lib/astc-encoder (db359593) (@null)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@null)

* Minor build tweaks (#407) (6a38a069) (@null)

### Tools

* Farewell GYP. :-( (f1f04a7e) (@null)

* Miscellaneous fixes (#558) (66f6d750) (@null)

* Add JNI component and integrate Java build & test with CMake (#556) (e29e0996) (@null)

* Fix non-clang warnings (#549) (4e7e40a0) (@null)

* Fix VS warnings (#544) (8c6b3571) (@null)

* Remove image.hpp dependency (#542) (9fde96b9) (@null)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@null)

* Improve Astc & BasisU normal map support (#493) (2d6ff949) (@null)

* Validate BasisU Transcode (#532) (39e2d96e) (@null)

* Fix mismatched errors for required and optional index entries. (b8786496) (@null)

* fix missing -w flag for ktx2check (eade072d) (@null)

* Remove transferFunction from astc options (#482) (1f085d30) (@null)

* Ensure NUL on end of 3d orientation. (74501ef3) (@null)

* Support array and 3d textures. (#468) (b0532530) (@null)

* Fix checks for mismatched image attributes. (#466) (4eca0ef3) (@null)

* Add more astc tests (#460) (14284e7d) (@null)

* Add astc support (#433) (da435dee) (@null)

* macOS Apple Silicon support (#415) (ebab2ea8) (@null)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@null)

* Fix validation errors (#417) (78cd2b01) (@null)



### JS Wrappers

* Farewell GYP. :-( (f1f04a7e) (@null)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@null)

* Raise warning levels to /W4 & -Wall -Wextra (#418) (ca6f6e7d) (@null)

### Java Wrapper

* Miscellaneous fixes (#558) (66f6d750) (@null)

* Add JNI component and integrate Java build & test with CMake (#556) (e29e0996) (@null)

* Fix warnings in JNI library and update to latest libktx API. (#548) (6f98b3c4) (@null)

* Update to Basis 1.16.1 (#541) (cb45eadc) (@null)

* Feature: Java bindings for libktx (#481) (a7159924) (@null)


