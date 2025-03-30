<!-- Copyright 2025, The Khronos Group Inc. -->
<!-- SPDX-License-Identifier: Apache-2.0 -->
Release Notes
=============
## Version 4.4
### Summary

* Aligns with [KTX Specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html) Revision 4 and [Khronos Data Format Specification](https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html) version 1.4.
* Adds new tools and features to the KTX suite. Of particular note is
  `ktx compare`.
* Has a largely rewritten Javascript binding which gives access to read _and_
  write functionality of *libktx* and includes an expanded test suite.
* Has a refactored and greatly improved Java binding thanks to @javagl.

With the new tools and features the KTX tool suite now provides a superset of the functionality found in the legacy tools with the exception of `ktx2ktx2`. __The legacy tools, except `ktx2ktx2`, will be removed in Release 4.5. Adjust your workflows accordingly.__

### New Features in 4.4
#### Command Line Tools Suite

Two tools have been added to the `ktx` suite in v4.4 and two tools have significant new functionality.

| Tool | Description | Equivalent lgeacy tool |
| :--- | ----------- | ------------------- |
| `ktx compare` | Compare KTX2 files | - |
| `ktx deflate` | Deflate a KTX2 file with Zlib or Zstd | `ktxsc` |
| `ktx create`  | Added `--assign-texcoord-origin`, `--convert-texcoord-origin`, `--normalize`, `--scale` | `toktx` |
| `ktx encode`  | Can now encode a KTX2 file to an ASTC format | `ktxsc` |

#### libktx functions

* `ktxTexture2_DecodeAstc` decodes an ASTC format texture to an uncompressed format.

* `ktxLoadOpenGL` so an application can explicitly make the library load the GL function pointers, that it uses, using an application provided `GetProcAddress` function. Use of this is recommended on any platform but it is most helpful on platforms such as Fedora where generic function queries are unable to find OpenGL functions.

* `ktxTexture2_Write*` and `ktxTexture[12]_Destroy` functions are now part of the public API.

### Notable Changes in 4.4

#### Alignment with [KTX Specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html) Revision 4

* Transfer function handling in `ktx create` has been revamped to support the flexibility of using almost any transfer function with non-sRGB formats that was introduced in Revision 1 of the [KTX Specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html).

* `ktx create` now allows setting a primaries value of "none", `KTX_DF_PRIMARIES_UNSPECIFIED`, which is required for certain formats in Revision 4 of the [KTX Specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html). 

* *libktx* now retains the values of the `bytesPlane[0-7]` fields in the DFD when supercompressing files as required following alignment of [KTX Specification](https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html) Revision 4 with [Khronos Data Format Specification](https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html) version 1.4.  It will load supercompressed files with either zero or non-zero `bytesPlane` values. `ktx validator` will report a warning on files with zero `bytesPlane` values.

#### Command Line Tools Suite



The following functionality has changed.

| Tool | Changed |
| :--- | ------- |
| `ktx create` | `--assign-oetf` → `--assign-tf`\*, <br/> `--convert-oetf` → `--convert-tf`.\*<br/> `--compare-ssim` and `--compare-psnr` can be used when encoding to ASTC.<br/>Non-raw input images can be resized by specifying the desired size with `--width` or `--height`. |  |
| `ktx encode` | `--compare-ssim` and `--compare-psnr` can be used when encoding to ASTC. |

\* Options renamed due to revamped functionality.

#### Javascript binding

The Javascript binding has been almost completely rewritten to support the read *and* write functionality of *libktx*. A test suite has been added that also serves as an example of how to use the functions.

* The name of the factory function for creating the ktx module has been changed from `LIBKTX` to `createKtxModule` (and `createKtxReadModule`). The old name is provided as an alias. Be aware that using the alias corresponds to `createKtxModule` so you will get the full functionality.
* The `ktx` prefixes have been removed from the only existing class that had it, `ktxTexture`. None of the new classes have a prefix. `ktcTexture` is provided as an alias to `texture`. Recommended usage is to assign the module instance returned by the factory function to `window.ktx` or other `ktx` variable making the prefixes unnecessary.
* Enumerator names are now the same as the *libktx* names minus the `ktx{,_}` prefixes and `_e` suffixes for consistency and to make it easier to recall what the JS name will be.
* Similarly constant names are the *libktx* names minus all prefixes as an enum or class name must be used when referencing them in JS. For example `KTX_PACK_UASTC_LEVEL_FASTEST` is simply `LEVEL_FASTEST` and is referenced in JS as `pack_uastc_flag_bits.LEVEL_FASTEST`.

#### Java binding

The Java binding has been greatly improved by a large refactoring focussing on error handling, documentation, completeness, and usability improvements ( #886 ). This includes several breaking changes. Not all breaking changes can be listed here explicitly. But in all cases, it should be easy for clients to update their code to the new state. The most important changes for the Java Wrapper are listed below.

* Made deflateZstd, deflateZLIB, and createFromMemory available in the KtxTexture2 class.
* Fixed a bug where the KtxBasisParams#setInputSwizzle function caused data corruption.
* Increased the consistency of the naming and handling of constants:
    * all constant names are UPPER\_SNAKE\_CASE, omitting common prefixes;
    * classes that define constants offer a stringFor function that returns the string representation of a constant;
    * the `KtxCreateStorage` class was renamed to `KtxTextureCreateStorage`;
    * the `KtxPackUASTCFlagBits` class was renamed to `KtxPackUastcFlagBits`.
* Improved error handling:
    * when functions receive a parameter that is null, then this will no longer crash the JVM, but will throw a NullPointerException;
    * when setInputSwizzle receives invalid arguments, then an `IllegalArgumentException` will be thrown;
    * when one of the create... family of functions causes an error, meaning that the respective KtxTexture2 object cannot be created, then a `KtxException` will be thrown.
* Added JavaDocs, and enabled the generation of JavaDocs as part of the Maven build process.
* Internal improvements (like JNI field caching, and avoiding calling JNI functions with pending exceptions).
* Supercompression functions exposed in the binding.
* `ktxTexture_GLUpload function exposed in the binding.

Massive thanks to @javagl for this work.


#### ASTC Encoder

* Updated to 5.2.0.

### Known Issues

* Files deflated with zlib using *libktx* compiled with GCC and run on x86\_64 may not be bit-identical with those using *libktx* compiled with GCC and run on arm64.

* Users making Basis Universal encoded or GPU block compressed textures for WebGL must be aware of WebGL restrictions with regard to texture size and may need to resize input images appropriately before using the `ktx create` tool, or use the `--resize` feature of the old `toktx` tool to produce an appropriately sized texture. In general, the dimensions of block compressed textures must be a multiple of the block size in WebGL and for WebGL 1.0 textures must have power-of-two dimensions. Additional portability restrictions apply for glTF per the _KHR\_texture\_basisu_ extension which can be verified using the `--gltf-basisu` command-line option of the new `ktx validate` tool.

* Basis Universal encoding results (both ETC1S/LZ and UASTC) are non-deterministic across platforms. Results are valid but level sizes and data will differ slightly.  See [issue #60](https://github.com/BinomialLLC/basis_universal/issues/60) in the basis_universal repository.

* UASTC RDO results differ from run to run unless multi-threading or RDO multi-threading is disabled. In `toktx` use `--threads 1` for the former or `--uastc_rdo_m` for the latter. As with the preceeding issue results are valid but level sizes will differ slightly. See [issue #151](https://github.com/BinomialLLC/basis_universal/issues/151) in the basis_universal repository.

* Neither the Vulkan nor GL loaders support depth/stencil textures.

### Changes since v4.3.2 (by part)
### libktx

* Fix memory leaks. (#1003) (0f0e06519) (@MarkCallow)

* Fix issues with ktxLoadOpenGL documentation. (0c1922d89) (@MarkCallow)

* Fix incorrect data types in libktx\_mainpage.md examples (#992) (b35a47811) (@Lephar)

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

* Add resize and scale support to create #999 (75d8d04bb) (@MarkCallow)

* Update for new commands and features. (350a63199) (@MarkCallow)

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

* Set up workflow to build just the documentation. (#989) (a45ffed4d) (@MarkCallow)

* Workaround removal of Python virtualenv in Actions runner 20240929.1. (#950) (e30405729) (@MarkCallow)

* Bump setuptools from 69.0.2 to 70.0.0 in /interface/python\_binding (#930) (f389108ff) (@dependabot[bot])

* Update R16G16\_S10\_5\_NV format to R16G16\_SFIXED5\_NV (#921) (7f3adf1b5) (@MarkCallow)


