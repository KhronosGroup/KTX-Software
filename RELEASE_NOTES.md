<!-- Copyright 2020, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-beta5
### New Features

* The build system now supports building a version `libktx` with only functionality for reading KTX files.

* The build system now has a configuration option to build a static library on Linux, macOS and Windows.

### Notable Changes

* A bug in the swizzle functionality internal to `libktx` has been fixed.
This bug caused some textures to be improperly swizzled before
encoding to BasisLZ/ETC1S and UASTC. The net result is that R textures were swizzled to R,0,0,255 instead of R,R,R,255 and RG textures or those with `separateRGToRGB_A` specified were swlzzled to R,R,0,255 instead of R,R,R,G.

* A bug that prevented zstd compressed textures with packed image formats from being loaded has been fixed.

### Known Issues

* Users making Basisu encoded or block compressed textures for WebGL
must be aware of WebGL restrictions with regard to texture size and
may need to resize images appropriately using the --resize feature
of `toktx`.  In general the dimensions of block compressed textures
must be a multiple of the block size and, if
`WEBGL_compressed_texture_s3tc` on WebGL 1.0 is expected to be one
of the targets, then the dimensions must be a power of 2.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are
non-deterministic across platforms. Results are valid but level
sizes and data will differ slightly.
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60)
in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
is disabled. As with the preceeding issue results are valid but
level sizes will differ slightly.
See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

### Changes since v4.0.0-beta4 (by part)
### libktx

* Move swizzle def to write exports. Fix VS warnings. (8942c6d4) (@MarkCallow)

* Export swizzzle\_to\_rgba. (f37f8772) (@MarkCallow)

* Fix swizzle\_to\_rgba and ETC1S channel ids. (1bb78e9d) (@MarkCallow)

* Match examples to actual usage (#339) (02423933) (@ErixenCruz)

* fix: Linking static ktx\_read now succeeds on Windows. Fixed by splitting up write-related exported symbols (that are not part of ktx\_read) in separate .def file. (#336) (6822201b) (@atteneder)

* fix: double "the" typo (#333) (d5d5708f) (@atteneder)

* Fix Writing BasisLZ/ETC1S example. Fixes #329. (19785767) (@MarkCallow)

* KTX read-only and static libraries (#324) (58c8e981) (@atteneder)

* fixed some typos in doc strings (#322) (c92b946b) (@atteneder)

* Recreate bytesPlane0 for supercompressed textures. (#321) (07a2ea9f) (@MarkCallow)

### Tools

* KTX read-only and static libraries (#324) (58c8e981) (@atteneder)

* fix: forcing a transfer function with multiple mipmap levels provided (#323) (6f32ea8f) (@atteneder)

* fixed some typos in doc strings (#322) (c92b946b) (@atteneder)






