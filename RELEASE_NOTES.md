<!-- Copyright 2020, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.0.0-beta4
### New Features

* `toktx` now supports 16-bit per component images as input for
Basis Universal encoding. Previously they could only be used to
create 16-bit format textures. It also supports using paletted
images as input. These will be expanded to RGB8 or RGBA8 depending
on presence of alpha.

* The WASM modules for the libktx and msc\_basis\_transcoder JS
bindings now include the BC7 and ETC_RG11 transcoders.

### Notable Changes

* A bug in `ktx2check` that caused some files with invalid data format descriptors to be passed as valid has been fixed.

* `CompressBasisEx` in `libktx` now requires explicit setting of
the `compressionLevel` in its `params` argument. To get the same
behavior as before callers should set this field to
`KTX_DEFAULT_ETC1S_COMPRESSION_LEVEL`.

* The JS wrappers are now compiled with `-O3` optimization leading to about a 2x performance increase at the cost of a small size increase.

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
See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in
the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading
is disabled. As with the preceeding issue results are valid but
level sizes ``will differ slightly.
See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151)
in the basis_universal repository.

### Changes since v4.0.0-beta3 (by part)
### libktx

* Implement KTXwriterScParams metadata (#315) (082d1b67) (@MarkCallow)

* Fix: identifier printing could overrun the identifier (#306) (5182a5e3) (@MarkCallow)

* Bring repository into REUSE compliance (#291) (3c1fa2ab) (@oddhack)

* Update for latest vulkan_core.h. (0062e172) (@MarkCallow)

* git subrepo pull (merge) lib/dfdutils (5b20f0f9) (@MarkCallow)

* git subrepo pull (merge) lib/dfdutils (355fce8c) (@MarkCallow)

* Fix parent which changed due to a squash merge. (020e43e9) (@MarkCallow)

* Don't set dllexport outside libktx. (32a1a287) (@MarkCallow)

* Require explicit setting of ktxBasisParams.compressionLevel. (46bdc7cc) (@MarkCallow)

* Simplify --qlevel. Remove --no_multithreading. Fixes #275. (da5c204a) (@MarkCallow)

* Support PNG files with only gAMA and cHRM chunks. (#282) (0d851050) (@MarkCallow)

* Check support of enough levels & layers for format. (10ce7454) (@MarkCallow)

* Return early on empty hashlist. (fc73f886) (@MarkCallow)

* Fix compile and Doxygen warnings. (2c40ba4d) (@MarkCallow)

### Tools

* Implement KTXwriterScParams metadata (#315) (082d1b67) (@MarkCallow)

*  Fixed loading PPM files where maxval in not equal to 255 or 65535 for 2 byte color component (#311) (1259580f) (@kacprzak)

* Fix: inverted memcmp result cause some invalid files to pass. Fixes #309 (#310) (9a0a12bf) (@MarkCallow)

* Fix: identifier printing could overrun the identifier (#306) (5182a5e3) (@MarkCallow)

* Fixed levelCount check when levelCount is 0 (d85bdb66) (@pdaehne)

* Fixed missing check for bytes/plane == 0 when supercompressing standard Vulkan texture formats (#299) (f5e05425) (@pdaehne)

* Fix for wrong error message in ktx2check (#297) (96e41fdc) (@pdaehne)

* Bring repository into REUSE compliance (#291) (3c1fa2ab) (@oddhack)

* Update for latest vulkan_core.h. (0062e172) (@MarkCallow)

* fix: By default create 1D texture when height == 1 (a3971738) (@kacprzak)

* Simplify --qlevel. Remove --no_multithreading. Fixes #275. (da5c204a) (@MarkCallow)

* Support PNG files with only gAMA and cHRM chunks. (#282) (0d851050) (@MarkCallow)

* Fix more MSVS compile warnings. (e6cc8963) (@MarkCallow)

* Support 16-bit and paletted images. (8283ea50) (@MarkCallow)

* Remove PreAllocator<> and std::vector hacks from ImageT (d1e1b8f7) (@zeux)

* Fix assertion in MSVC Debug (cd62c227) (@zeux)

* Derive toktx from scapp/ktxapp & capture sc args in metadata (#256) (67855d1e) (@MarkCallow)

* Fix compile and Doxygen warnings. (2c40ba4d) (@MarkCallow)



### JS Wrappers

* Bring repository into REUSE compliance (#291) (3c1fa2ab) (@oddhack)

* Add missing BC7_RGBA enum. Deprecate others. (571973e5) (@MarkCallow)


