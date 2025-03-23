<!-- Copyright 2025, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.4.0
### New Features in Version 4.4
#### Command Line Tools Suite

Several tools have been added to the `ktx` suite v4.4.

| Tool | Description | Equivalent old tool |
| :--- | ----------- | ------------------- |
| `ktx compare` | Compare KTX2 files | - |
| `ktx deflate` | Deflate a KTX2 file with Zlib or Zstd | `ktxsc` |

The old tools, except `ktx2ktx2`, will be removed in a subsequent release soon.

### Notable Changes in v4.4
* The Java wrapper has been greatly improved thanks to @javagl.
* A new `libktx` function has been added so an application can explicitly make the library load the GL function pointers, that it uses, using an application provided `GetProcAddress` function. This is for platforms such as Fedora where generic function queries are unable to find OpenGL functions.

### Known Issues

* Some image bits in output files encoded to ASTC, ETC1S/Basis-LZ or UASTC on arm64 devices may differ from those encoded from the same input images on x86_64 devices. The differences will not be human visible and will only show up in bit-exact comparisons. 

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using the `ktx create` tool, or use the `--resize` feature of the old `toktx` tool to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of the new `ktx validate` tool.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.


### Changes since v4.3.2 (by part)
### libktx

* Set bytes planes to non-zero for supercompressed formats. (#988) (d4cad5f85) (@MarkCallow)

* Fix memory leak. (#991) (e0cf193d4) (@MarkCallow)

* Add normalise option to command create (#977) (2f691839a) (@wasimabbas-arm)

* Make ktxTexture2\_Write functions public. (#985) (b330a2ee9) (@MarkCallow)

* ktx create: Update transfer function handling (#982) (7356c0d74) (@MarkCallow)

* Add vkformat checks to ktxTexture2\_DecodeAstc (#967) (aa6af91c6) (@wasimabbas-arm)

* Fix ktxTexture2\_DecodeAstc error returns and document them. (#961) (b6190046b) (@MarkCallow)

* Fix ktxTexture2\_DecodeAstc documentation. (c16537c89) (@MarkCallow)

* Encode astc support (#952) (d1ad5cdc4) (@wasimabbas-arm)

* Expose ktxTexture2\_GetImageOffset. (#954) (99c324fff) (@MarkCallow)

* JNI improvements (#886) (2ee4332a1) (@javagl)

* Add clang-format support (#913) (110f049fd) (@MathiasMagnus)

* Fix issues newly flagged by Doxygen 1.11.0 (#925) (19c2f75ab) (@MarkCallow)

* docs: clarify pointer lifetimes for ktxTexture\_SetImageFromMemory (#917) (b2cad1ad4) (@mjrister)

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)

* Add JS bindings for full libktx (#874) (562509c7f) (@aqnuep)

* Fixes for strnlen and unicode string literals (#916) (759f49d95) (@rGovers)

* INVALID\_OPERATION if generateMipmaps set when block compressing. (e5585e34f) (@MarkCallow)

* Expose ktxTexture[12]\_Destroy. (7132048c1) (@MarkCallow)

* Move self-hosted dependencies to `external` folder (#909) (5fc739c8f) (@MathiasMagnus)

* Fix various issues surfaced by fuzzer (#900) (6063e470f) (@florczyk-fb)

* Expose supercompression functions via JNI (#879) (7abffbdbe) (@javagl)

* Add function for app to explicitly make libktx load its GL function pointers. (#894) (0bca55aa7) (@MarkCallow)

* Miscellaneous fixes (#893) (e31b3e5b8) (@MarkCallow)

* Minor documentation fixes (#890) (e7d2d7194) (@javagl)

* Merge ktx compare to main (#868) (6fcd95a7f) (@aqnuep)

### Tools

* Set bytes planes to non-zero for supercompressed formats. (#988) (d4cad5f85) (@MarkCallow)

* Add normalise option to command create (#977) (2f691839a) (@wasimabbas-arm)

* ktx create: Update transfer function handling (#982) (7356c0d74) (@MarkCallow)

* Add --{assign.convert}-texcoord-origin option to ktx create (#976) (91e61d6fa) (@MarkCallow)

* fix: INSTALL\_RPATH for linux using lib64 (#975) (ba9783356) (@MinGyuJung1996)

* Add vkformat checks to ktxTexture2\_DecodeAstc (#967) (aa6af91c6) (@wasimabbas-arm)

* Fix crash on loading EXR 2.0 input (#973) (7a8cd47de) (@MathiasMagnus)

* Encode astc support (#952) (d1ad5cdc4) (@wasimabbas-arm)

* Always use same method to set language version. (#947) (3439dafa7) (@MarkCallow)

* Fix: ensure ktxdiff runs on macOS and enable CTS in Linux arm64 CI. (#946) (ffb915299) (@MarkCallow)

* Fixing build on Ubuntu 24.04 w/ Clang 20 (#936) (33813d72a) (@Honeybunch)

* Fix issues newly flagged by Doxygen 1.11.0 (#925) (19c2f75ab) (@MarkCallow)

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)

* Update astc-encoder to 4.8.0 (subrepo pull). (#918) (7fb646cd3) (@MarkCallow)

* Move self-hosted dependencies to `external` folder (#909) (5fc739c8f) (@MathiasMagnus)

* Fix man page issues introduced by PR #817 (#903) (0d1ebc1d0) (@MarkCallow)

* Refactor common options (#817) (0a16df82e) (@wasimabbas-arm)

* Add `ktx deflate` command (#896) (da97911f9) (@MarkCallow)

* Fixes to CLI error messages (#891) (eac4b9878) (@aqnuep)

* Minor documentation fixes (#890) (e7d2d7194) (@javagl)

* Merge ktx compare to main (#868) (6fcd95a7f) (@aqnuep)

* Clarify description of --format argument for create when used with --encode (#873) (5414bd0eb) (@aqnuep)



### JS Bindings

* Align naming (#938) (a3cf1d506) (@MarkCallow)

* Remove long ago deprecated items. (#926) (b1a115f3a) (@MarkCallow)

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)

* Add JS bindings for full libktx (#874) (562509c7f) (@aqnuep)

* Move self-hosted dependencies to `external` folder (#909) (5fc739c8f) (@MathiasMagnus)

### Java Binding

* Java swizzle and constant fixes (#972) (3038f7cae) (@javagl)

* Add GL upload function to Java interface (#959) (bd8ae318f) (@javagl)

* Fix JNI function name for `getDataSizeUncompressed` (#957) (f3902db35) (@javagl)

* JNI improvements (#886) (2ee4332a1) (@javagl)

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)

* Expose supercompression functions via JNI (#879) (7abffbdbe) (@javagl)

* Properly convert Java char to C char in inputSwizzle (#876) (cc5648532) (@javagl)

### Python Binding

* Workaround removal of Python virtualenv in Actions runner 20240929.1. (#950) (e30405729) (@MarkCallow)

* Bump setuptools from 69.0.2 to 70.0.0 in /interface/python\_binding (#930) (f389108ff) (@dependabot[bot])

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)


